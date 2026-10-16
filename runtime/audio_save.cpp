// XXIII-W save-data surface
// XXIII-C audio path
// XXII-K: save lifecycle surface (create/list/load)
// XXII-F: audio ring / notify surface
// Phase XVI audio + XVIII PS3-native save (linked with ps3rt)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#endif

static int g_audio_inited = 0;
#if defined(_WIN32)
static HWAVEOUT g_wo = nullptr;
static WAVEHDR g_wh[4]{};
static int16_t g_pcm[4][2048]; // 4 buffers x 1024 stereo frames
static int g_wh_idx = 0;
static uint32_t g_audio_block = 256;
static uint64_t g_audio_read_idx = 0;
static uint64_t g_port_addr = 0; // guest EA of ring (if known)
static uint32_t g_port_nblocks = 8;
static int g_port_open = 0;
// G+K: ring + multi-port (up to 8)
static uint32_t g_ring_write = 0;
static uint32_t g_ring_read = 0;
static uint32_t g_ring_nblocks = 8;
static uint64_t g_notify_key = 0;
static int g_ports_active = 0;
static uint32_t g_port_flags[8] = {};
static uint32_t g_port_ring_w[8] = {};
static uint32_t g_port_ring_r[8] = {};
static int g_current_port = 0;

#endif

static char g_game_name[128] = "GOW3";
static char g_save_root[512] = {};

extern "C" {

PS3RT_API void ps3rt_set_game_name(const char* name) {
    if (!name || !name[0]) return;
    std::snprintf(g_game_name, sizeof(g_game_name), "%s", name);
    g_save_root[0] = 0;
}

PS3RT_API const char* ps3rt_get_game_name(void) {
    return g_game_name;
}

static void save_ensure_root() {
    if (g_save_root[0]) return;
#if defined(_WIN32)
    char docs[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_PERSONAL, nullptr, 0, docs)))
        std::snprintf(g_save_root, sizeof(g_save_root), "%s\\%s\\PS3_SAVEDATA", docs, g_game_name);
    else
        std::snprintf(g_save_root, sizeof(g_save_root), ".\\%s\\PS3_SAVEDATA", g_game_name);
    char path[512];
    std::snprintf(path, sizeof(path), "%s", g_save_root);
    for (char* p = path; *p; ++p) {
        if (*p == '\\' || *p == '/') {
            char ch = *p; *p = 0; CreateDirectoryA(path, nullptr); *p = ch;
        }
    }
    CreateDirectoryA(path, nullptr);
#else
    const char* home = getenv("HOME");
    std::snprintf(g_save_root, sizeof(g_save_root), "%s/Documents/%s/PS3_SAVEDATA", home ? home : ".", g_game_name);
#endif
    ps3rt_log("[save] root=%s (PS3-native only; no savestates)\n", g_save_root);
}

static void audio_host_open() {
#if defined(_WIN32)
    if (g_wo) return;
    WAVEFORMATEX wfx{};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 48000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 4;
    wfx.nAvgBytesPerSec = 48000 * 4;
    if (waveOutOpen(&g_wo, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        g_wo = nullptr; ps3rt_log("[audio] G/K waveOutOpen failed — silent HLE\n"); return;
    }
    for (int i = 0; i < 4; ++i) {
        std::memset(g_pcm[i], 0, sizeof(g_pcm[i]));
        g_wh[i].lpData = (LPSTR)g_pcm[i];
        g_wh[i].dwBufferLength = sizeof(g_pcm[i]);
        waveOutPrepareHeader(g_wo, &g_wh[i], sizeof(g_wh[i]));
    }
    ps3rt_log("[audio] G/K host waveOut 48kHz stereo ready (4 buffers)\n");
#endif
}

// Push interleaved stereo s16 samples to waveOut
static void audio_queue_s16(const int16_t* samples, size_t nframes) {
#if defined(_WIN32)
    if (!g_wo || !samples || nframes == 0) return;
    size_t left = nframes;
    const int16_t* src = samples;
    while (left > 0) {
        WAVEHDR* h = &g_wh[g_wh_idx];
        if (h->dwFlags & WHDR_INQUEUE) {
            // all busy — drop
            break;
        }
        size_t cap = sizeof(g_pcm[0]) / 4; // frames
        size_t n = left < cap ? left : cap;
        std::memcpy(g_pcm[g_wh_idx], src, n * 4);
        if (n < cap) std::memset((uint8_t*)g_pcm[g_wh_idx] + n * 4, 0, (cap - n) * 4);
        h->dwBufferLength = (DWORD)(n * 4);
        waveOutWrite(g_wo, h, sizeof(*h));
        g_wh_idx = (g_wh_idx + 1) & 3;
        src += n * 2;
        left -= n;
    }
#else
    (void)samples; (void)nframes;
#endif
}

PS3RT_API void ps3rt_audio_tick(void) {
    audio_host_open(); // XXIII-C

#if defined(_WIN32)
    if (!g_wo || !g_audio_inited) return;
    // Keep at least one silent buffer if nothing queued (prevent underrun click)
    int queued = 0;
    for (int i = 0; i < 4; ++i) if (g_wh[i].dwFlags & WHDR_INQUEUE) ++queued;
    if (queued == 0) {
        WAVEHDR* h = &g_wh[g_wh_idx];
        std::memset(g_pcm[g_wh_idx], 0, sizeof(g_pcm[0]));
        h->dwBufferLength = sizeof(g_pcm[0]);
        waveOutWrite(g_wo, h, sizeof(*h));
        g_wh_idx = (g_wh_idx + 1) & 3;
    }
    ++g_audio_read_idx;
#endif
}

PS3RT_API int ps3rt_audio_dispatch(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    // cellAudioInit
    if (nid == 0x0B168F92 || nid == 0x0B2A7777 || nid == 0x0E4DF449) {
        g_audio_inited = 1; audio_host_open();
        ps3rt_log("[audio] G/K cellAudioInit\n"); c->gpr[3] = 0; return 0;
    }
    // Port open / create / start
    if (nid == 0x5B1E2C73 || nid == 0x566966C0 || nid == 0x65C27676 || nid == 0x4129FE2C) {
        if (!g_audio_inited) { g_audio_inited = 1; audio_host_open(); }
        g_port_open = 1;
        if (g_ports_active < 8) {
            g_current_port = g_ports_active;
            g_port_flags[g_ports_active] |= 1u;
            ++g_ports_active;
            static int spo=0; if(spo<8){ps3rt_log("[audio] G/K port open id=%d active=%d\n", g_current_port, g_ports_active);++spo;}
        }
        // r3 may be port id out; write 0
        if (c->gpr[3] && c->gpr[3] + 4 <= 512ull*1024*1024) {
            // leave; game usually passes out pointer in r4/r3
        }
        c->gpr[3] = 0; return 0;
    }
    // Port stop / quit
    if (nid == 0xDDC2AB95 || nid == 0x4129FE2A) {
        c->gpr[3] = 0; return 0;
    }
    // GetPortConfig — fill structure with block size / address if possible
    if (nid == 0x07ADD381) {
        uint64_t out = c->gpr[4] ? c->gpr[4] : c->gpr[3];
        uint8_t* mem = ps3rt_memory();
        if (mem && out && out + 0x20 <= 512ull*1024*1024) {
            // minimal CellAudioPortConfig-like: nChannel=2, nBlock=8, nAddr
            auto be32 = [](uint8_t* p, uint32_t v) {
                p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v;
            };
            be32(mem + out + 0, 2);           // nChannel
            be32(mem + out + 4, g_port_nblocks);
            be32(mem + out + 8, g_audio_block);
            // nAddr leave 0 unless we allocated — games often allocate themselves
        }
        c->gpr[3] = 0; return 0;
    }
    // AddData / Add2chData — r3=port, r4=src EA, r5=samples count
    if (nid == 0x2B9BD9AD || nid == 0x377E0CD9) {
        uint64_t src = c->gpr[4];
        uint32_t nsamp = (uint32_t)c->gpr[5];
        uint8_t* mem = ps3rt_memory();
        if (mem && src && nsamp > 0 && src + (uint64_t)nsamp * 4 <= 512ull*1024*1024) {
            if (nsamp > 4096) nsamp = 4096;
            // guest often float32 planar or interleaved float — try detect:
            // HLE: treat as interleaved float [-1,1] stereo and convert
            const float* f = (const float*)(mem + src);
            static int16_t conv[8192];
            uint32_t nframes = nsamp; // treat nsamp as frames for Add2ch
            if (nid == 0x2B9BD9AD) nframes = nsamp; // may be total samples
            if (nframes > 4096) nframes = 4096;
            for (uint32_t i = 0; i < nframes * 2 && i < 8192; ++i) {
                float x = f[i];
                if (x > 1.f) x = 1.f; if (x < -1.f) x = -1.f;
                conv[i] = (int16_t)(x * 32767.f);
            }
            audio_queue_s16(conv, nframes);
            static int s_add = 0;
            if (s_add < 8) {
                int pid = g_current_port & 7;
            g_port_ring_w[pid] = (g_port_ring_w[pid] + 1) & 15;
            g_ring_write = (g_ring_write + 1) % (g_ring_nblocks ? g_ring_nblocks : 8);
            ps3rt_log("[audio] G/K AddData port=%d frames=%u ring_w=%u\n", pid, nframes, g_port_ring_w[pid]);
                ++s_add;
            }
        }
        c->gpr[3] = 0; return 0;
    }
    // SetNotifyEventQueue / CreateNotifyEventQueue
    if (nid == 0xC20EE39D || nid == 0x2BEF9CDE) {
        g_notify_key = c->gpr[3];
        static int s_nq = 0;
        if (s_nq < 6) { ps3rt_log("[audio] G/K SetNotifyEventQueue key=0x%llx\n", (unsigned long long)g_notify_key); ++s_nq; }
        c->gpr[3] = 0; return 0;
    }
    c->gpr[3] = 0;
    return 0;
}

static int save_write_file(const char* dir_name, const char* file_name, const void* data, size_t len) {
    save_ensure_root();
    char dir[600], path[700];
#if defined(_WIN32)
    std::snprintf(dir, sizeof(dir), "%s\\%s", g_save_root, dir_name ? dir_name : "SLOT0");
    CreateDirectoryA(dir, nullptr);
    std::snprintf(path, sizeof(path), "%s\\%s", dir, file_name ? file_name : "PARAM.SFO");
#else
    std::snprintf(dir, sizeof(dir), "%s/%s", g_save_root, dir_name ? dir_name : "SLOT0");
    std::snprintf(path, sizeof(path), "%s/%s", dir, file_name ? file_name : "PARAM.SFO");
#endif
    FILE* f = std::fopen(path, "wb");
    if (!f) { ps3rt_log("[save] write fail %s\n", path); return -1; }
    if (data && len) std::fwrite(data, 1, len, f);
    std::fclose(f);
    ps3rt_log("[save] wrote %s (%zu bytes)\n", path, len);
    return 0;
}

PS3RT_API int ps3rt_save_dispatch(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    save_ensure_root();
    // P: lifecycle — create / list / load / save / delete / exists
    // create / save (PARAM.SFO + slot)
    if (nid == 0x04C06FC2 || nid == 0x21CCD0B0 || nid == 0xEDB7727B ||
        nid == 0x95AEBDAB || nid == 0x52CBC4FC) {
        uint8_t buf[256] = {};
        std::memcpy(buf, "\0PSF", 4);
        std::snprintf((char*)buf + 0x14, 64, "%s", g_game_name);
        save_write_file("SLOT0", "PARAM.SFO", buf, sizeof(buf));
        save_write_file("SLOT0", "ICON0.PNG", nullptr, 0);
        static int s_sv = 0;
        if (s_sv < 12) { ps3rt_log("[save] P create/save nid=0x%08X -> %s\\SLOT0\n", nid, g_save_root); ++s_sv; }
        c->gpr[3] = 0; return 0;
    }
    // load / open
    if (nid == 0xF6488C08 || nid == 0x39D6C2BB || nid == 0x24793432) {
        static int s_ld = 0;
        if (s_ld < 12) { ps3rt_log("[save] P load/open nid=0x%08X from %s\n", nid, g_save_root); ++s_ld; }
        c->gpr[3] = 0; return 0;
    }
    // list / getdir / enumerate — report 1 slot
    if (nid == 0x21965478 || nid == 0x1AF0148E || nid == 0xEDC97180) {
        uint64_t out = c->gpr[3];
        // if pointer, write count=1
        uint8_t* mem = ps3rt_memory();
        if (mem && out && out + 4 <= 512ull*1024*1024) {
            mem[out] = 0; mem[out+1] = 0; mem[out+2] = 0; mem[out+3] = 1;
        }
        static int s_ls = 0;
        if (s_ls < 8) { ps3rt_log("[save] P list/enum nid=0x%08X slots=1\n", nid); ++s_ls; }
        c->gpr[3] = 0; return 0;
    }
    // delete
    if (nid == 0x48418D11 || nid == 0x27978D77) {
        static int s_dl = 0;
        if (s_dl < 8) { ps3rt_log("[save] P delete nid=0x%08X\n", nid); ++s_dl; }
        c->gpr[3] = 0; return 0;
    }
    // stat / get size / exists — success empty
    if (nid == 0x5CB9BCFF || nid == 0x22AC00B8 || nid == 0x8B7B4BF8) {
        c->gpr[3] = 0; return 0;
    }
    c->gpr[3] = 0;
    return 0;
}

PS3RT_API void ps3rt_save_boot_notice(void) {
    save_ensure_root();
}

} // extern "C"
