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
static WAVEHDR g_wh{};
static int16_t g_silence[1024];
#endif

static char g_game_name[128] = "GOW3";
static char g_save_root[512] = {};

extern "C" {

PS3RT_API void ps3rt_set_game_name(const char* name) {
    if (!name || !name[0]) return;
    std::snprintf(g_game_name, sizeof(g_game_name), "%s", name);
    g_save_root[0] = 0;
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
        g_wo = nullptr; ps3rt_log("[audio] waveOutOpen failed — silent HLE\n"); return;
    }
    std::memset(g_silence, 0, sizeof(g_silence));
    g_wh.lpData = (LPSTR)g_silence;
    g_wh.dwBufferLength = sizeof(g_silence);
    waveOutPrepareHeader(g_wo, &g_wh, sizeof(g_wh));
    ps3rt_log("[audio] host waveOut 48kHz stereo ready\n");
#endif
}

PS3RT_API void ps3rt_audio_tick(void) {
#if defined(_WIN32)
    if (!g_wo || !g_audio_inited) return;
    // keep a silent buffer queued so waveOut stays alive
    if (!(g_wh.dwFlags & WHDR_INQUEUE)) {
        waveOutWrite(g_wo, &g_wh, sizeof(g_wh));
    }
#endif
}

PS3RT_API int ps3rt_audio_dispatch(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    if (nid == 0x0B2A7777 || nid == 0x0E4DF449) { // cellAudioInit variants
        g_audio_inited = 1; audio_host_open();
        ps3rt_log("[audio] cellAudioInit\n"); c->gpr[3] = 0; return 0;
    }
    if (nid == 0x4129FE2C || nid == 0x5B1E2C73) { // cellAudioPortOpen / PortStart
        if (!g_audio_inited) { g_audio_inited = 1; audio_host_open(); }
        c->gpr[3] = 0; return 0;
    }
    // other cellAudio* → success stub
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
    // Minimal PARAM.SFO-ish header so games see a slot
    if (nid == 0x04C06FC2 || nid == 0x21CCD0B0 || nid == 0xEDB7727B) {
        uint8_t buf[256] = {};
        std::memcpy(buf, "\0PSF", 4);
        std::snprintf((char*)buf + 0x14, 64, "%s", g_game_name);
        save_write_file("SLOT0", "PARAM.SFO", buf, sizeof(buf));
        save_write_file("SLOT0", "ICON0.PNG", nullptr, 0);
        ps3rt_log("[save] game SAVE nid=0x%08X -> %s\\SLOT0\n", nid, g_save_root);
        c->gpr[3] = 0; return 0;
    }
    if (nid == 0xF6488C08 || nid == 0x39D6C2BB) {
        ps3rt_log("[save] game LOAD nid=0x%08X from %s\n", nid, g_save_root);
        c->gpr[3] = 0; return 0;
    }
    if (nid == 0x48418D11) {
        ps3rt_log("[save] delete requested\n"); c->gpr[3] = 0; return 0;
    }
    c->gpr[3] = 0;
    return 0;
}

PS3RT_API void ps3rt_save_boot_notice(void) {
    save_ensure_root();
}

} // extern "C"
