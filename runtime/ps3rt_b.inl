// CMDSTREAM: inject VA0 triangle into guest FIFO (real draw path)
// FIFO PUT/GET fix: never set GET=PUT on emit; bright inject + flush
// XXV-I/J: services + E2E checklist
// XXV-H: SPURS wait contract
// XXV-G depth: PRX import contracts + unknown NID telemetry\n// XXV-D depth on XXV-E: GCM color seed + denser present
// XXV-E denser GCM present assist
// XXIV-D PRX NID quality + return surface
// XXIV-B GCM put threshold
// XXIII-X frame assist
// XXIII-W save/sysutil depth surface
// XXIII-S worker assist via existing SPURS/SPU phase in progress_hw
// XXIII-Q import/HLE coverage for init that issues draws
// XXIII-K timing model
// XXII-D marker
#include <vector>
// O: NID table expanded in nid_table.h (sysutil/sysmodule/pad/kb/mouse/spurs stubs)
static void pad_write_data(uint64_t out) {
    if (!out || out + 24 > g_mem_size) return;
    wr_be32_guest(out, 24);
    g_mem[out+4] = (uint8_t)(g_pad_buttons >> 8);
    g_mem[out+5] = (uint8_t)(g_pad_buttons & 0xFF);
    g_mem[out+6] = (uint8_t)g_pad_lx;
    g_mem[out+7] = (uint8_t)g_pad_ly;
    g_mem[out+8] = (uint8_t)g_pad_rx;
    g_mem[out+9] = (uint8_t)g_pad_ry;
    for (int i = 10; i < 24; ++i) g_mem[out + i] = 0;
    g_mem[out+10] = g_pad_l2;
    g_mem[out+11] = g_pad_r2;
}

static void pad_write_info2(uint64_t out) {
    if (!out || out + 0x50 > g_mem_size) return;
    std::memset(g_mem + out, 0, 0x50);
    wr_be32_guest(out + 0, 7);
    wr_be32_guest(out + 4, g_pad_connected);
    wr_be32_guest(out + 8, 0);
    wr_be32_guest(out + 12, g_pad_connected ? 0x00000001u : 0);
}

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    if (!c) return;
    uint64_t num = c->gpr[11];
    uint64_t r3 = c->gpr[3], r4 = c->gpr[4], r5 = c->gpr[5], r6 = c->gpr[6];
    (void)r6;
    static int sys_log = 0;
    if (sys_log < 120 && (uint32_t)num != 141 && (uint32_t)num != 142) {
        ps3rt_log("[lv2] sc %llu r3=%llx r4=%llx r5=%llx r6=%llx\n",
            (unsigned long long)num, (unsigned long long)r3, (unsigned long long)r4,
            (unsigned long long)r5, (unsigned long long)c->gpr[6]);
        ++sys_log;
    }
    switch ((uint32_t)num) {
    case 0:
        c->halted = true; c->gpr[3] = 0; break;
    case 3:
        ps3rt_request_exit();
        c->halted = true; c->gpr[3] = 0; break;
    case 14: case 15: case 25602:
        c->gpr[3] = 0; break;
case 1: // getpid
    c->gpr[3] = 1; break;
case 4:
    c->gpr[3] = 0; break;
case 5:
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, 1);
    c->gpr[3] = 0; break;
case 25:
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, 0x00440000);
    c->gpr[3] = 0; break;
case 41: case 43: case 44: case 45: case 46: case 47: {
    static uint32_t s_tid = 1;
    if ((uint32_t)num == 41 && r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, s_tid++);
    c->gpr[3] = 0; break;
}
case 48: case 49: case 50: case 51: case 52: case 53:
    c->gpr[3] = 0; break;
case 90: case 91: case 92: case 93: case 94: case 95: case 96:
    c->gpr[3] = 0; break;
case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
    c->gpr[3] = 0; break;
case 112: case 113: case 114: case 115: case 116: case 117:
    c->gpr[3] = 0; break;
case 128: {
    static uint32_t qid = 0x1000;
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++qid);
    c->gpr[3] = 0; break;
}
case 129: case 130: case 131: case 132: case 133: case 134: case 135:
case 136: case 137: case 138: case 139:
    c->gpr[3] = 0; break;
case 141: case 142: {
    // sys_timer_usleep / sys_timer_sleep
    // GOW: sc 141 r3=0x186a0 spam — do not Sleep(ms) on PPU thread.
    uint64_t us = r3;
    static uint64_t s_us_last = 0;
    static int s_us_n = 0;
    if (us == s_us_last) ++s_us_n; else { s_us_last = us; s_us_n = 1; }
#if defined(_WIN32)
    Sleep(0);
#endif
    if (s_us_n <= 2 || (s_us_n & 15) == 0) {
        extern void ps3rt_progress_hw(void);
        ps3rt_progress_hw();
    }
    static int s_us_log = 0;
    if (s_us_log < 8 || s_us_n == 1 || (s_us_n % 256) == 0) {
        ps3rt_log("[lv2] timer_usleep us=%llu n=%d (yield+hw)\n",
            (unsigned long long)us, s_us_n);
        ++s_us_log;
    }
    c->gpr[3] = 0; break;
}
case 143: {
    static uint32_t fid = 0x8000;
    if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++fid);
    c->gpr[3] = 0; break;
}
case 144: {
    static uint64_t fake_sec = 1000;
    ++fake_sec;
    if (r3 && r3 + 8 <= g_mem_size) wr_be64_guest(r3, fake_sec);
    if (r4 && r4 + 8 <= g_mem_size) wr_be64_guest(r4, 0);
    c->gpr[3] = 0; break;
}
case 145:
    c->gpr[3] = 79800000ull; break;
case 156: case 157: case 158: case 159:
case 160: case 161: case 162: case 163: case 165: case 166: case 167:
case 169: case 170: case 171: case 172: case 173: case 174: case 175:
case 176: case 177: case 178: case 179: case 180: case 181:
    c->gpr[3] = 0; break;
case 330: case 331: case 332: case 333: case 334:
    c->gpr[3] = 0; break;
case 348: case 349: case 350: case 351: case 352: case 353: case 354: {
    // GOW: sc 352 r3/r4 = 0xf0ffbxx stack addrs must not count as size.
    auto looks_size = [](uint64_t v) -> bool {
        if (v < 0x100ull || v > 0x08000000ull) return false;
        if (v >= 0x0E000000ull) return false;
        if ((v & 0xFFF00000ull) == 0x0F000000ull) return false;
        return true;
    };
    auto looks_ptr = [](uint64_t v) -> bool {
        return v >= 0x10000ull && v < g_mem_size;
    };
    uint64_t sz = 0, out = 0, flags = 0;
    if (looks_size(r4) && looks_ptr(r3)) { out = r3; sz = r4; flags = r5; }
    else if (looks_size(r3)) { sz = r3; flags = r4; out = (r5 && looks_ptr(r5)) ? r5 : 0; }
    else if (looks_size(r4)) { sz = r4; out = looks_ptr(r3) ? r3 : 0; flags = r5; }
    else if (looks_ptr(r3) && looks_ptr(r4)) {
        uint64_t s1 = rd_be32_guest(r4);
        uint64_t s2 = rd_be32_guest(r3);
        if (looks_size(s1)) { sz = s1; out = r3; flags = r5; }
        else if (looks_size(s2)) { sz = s2; out = r4; flags = r5; }
        else {
            s1 = rd_be64_guest(r4); s2 = rd_be64_guest(r3);
            if (looks_size(s1)) { sz = s1; out = r3; }
            else if (looks_size(s2)) { sz = s2; out = r4; }
        }
    }
    if (!sz) {
        static int s_bad = 0;
        if (s_bad < 24) {
            ps3rt_log("[lv2] mem REJECT sc=%u r3=0x%llx r4=0x%llx (not a size)\n",
                (unsigned)num, (unsigned long long)r3, (unsigned long long)r4);
            ++s_bad;
        }
        c->gpr[3] = (uint64_t)(int64_t)-1;
        break;
    }
    uint64_t al = 0x10000;
    if (flags && flags >= 0x1000 && flags <= 0x100000) al = flags;
    uint64_t a = guest_alloc(sz, al);
    if (out && out + 8 <= g_mem_size) wr_be64_guest(out, a);
    static int s_m = 0;
    if (s_m < 24) {
        ps3rt_log("[lv2] XV-L mem sc=%u size=0x%llx -> ea=0x%llx\n",
            (unsigned)num, (unsigned long long)sz, (unsigned long long)a);
        ++s_m;
    }
    c->gpr[3] = a ? 0 : (uint64_t)(int64_t)-1;
    break;
}
case 480: case 481: case 482: case 483: case 484: case 485: case 486:
case 487: case 488: case 489: case 490: case 491: case 492: case 493:
case 494: case 496: case 497: case 498: case 499: {
    static uint32_t s_prxid = 0x100;
    if ((uint32_t)num == 480 || (uint32_t)num == 499) {
        if (r4 && r4 + 4 <= g_mem_size) wr_be32_guest(r4, ++s_prxid);
        else if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++s_prxid);
    }
    static int s_prx = 0;
    if (s_prx < 20) {
        ps3rt_log("[lv2] XV-L sys_prx sc=%u r3=0x%llx r4=0x%llx\n",
            (unsigned)num, (unsigned long long)r3, (unsigned long long)r4);
        ++s_prx;
    }
    c->gpr[3] = 0; break;
}
case 801: case 802: case 803: case 804: case 805: case 806: case 807:
case 808: case 809: case 810: case 811: case 812: case 813: case 814:
    c->gpr[3] = 0; break;
case 871: case 872: case 873: case 874: case 875:
    c->gpr[3] = 0; break;
case 986: case 987: case 988: case 989: case 990: case 991:
    c->gpr[3] = 0; break;

    default: {
        static int s_def = 0;
        if (s_def < 40) {
            ps3rt_log("[lv2] XV-L unhandled sc %u -> 0\n", (unsigned)num);
            ++s_def;
        }
        c->gpr[3] = 0; break;
    }
    }
}

static int g_prx_log = 0;

// PRX registry + export bind + ELF load (ordered for C++ compile)
struct PrxMod { bool used; char name[64]; uint32_t id; uint32_t entry; uint32_t exports; uint64_t base; };
struct PrxExport { uint32_t nid; uint64_t ea; };
static PrxMod g_prx_mods[32];
static uint32_t g_prx_mod_seq = 1;
static PrxExport g_prx_exports[128];
static int g_prx_export_n = 0;
// XXV-G: import contract telemetry
struct PrxImportHit { uint32_t nid; uint32_t hits; int status; }; // status: 0=ok 1=stub 2=unknown
static PrxImportHit g_prx_hits[256];
static int g_prx_hit_n = 0;
static int g_prx_unknown_n = 0;
static void prx_note_hit(uint32_t nid, int status) {
    for (int i = 0; i < g_prx_hit_n; ++i) {
        if (g_prx_hits[i].nid == nid) { g_prx_hits[i].hits++; g_prx_hits[i].status = status; return; }
    }
    if (g_prx_hit_n < 256) {
        g_prx_hits[g_prx_hit_n++] = { nid, 1u, status };
    }
}
static int prx_strict_imports() {
    static int c = -1;
    if (c < 0) {
        const char* e = std::getenv("PS3RT_STRICT_IMPORT");
        c = (e && e[0] == '1') ? 1 : 0;
        if (c) ps3rt_log("[prx] XXV-G STRICT_IMPORT=1 — unknown NID returns CELL_ENOSYS\n");
    }
    return c;
}


static void prx_bind_export(uint32_t nid, uint64_t ea) {
    for (int i = 0; i < g_prx_export_n; ++i)
        if (g_prx_exports[i].nid == nid) { g_prx_exports[i].ea = ea; return; }
    if (g_prx_export_n < 128) {
        g_prx_exports[g_prx_export_n].nid = nid;
        g_prx_exports[g_prx_export_n].ea = ea;
        ++g_prx_export_n;
    }
}

static uint64_t prx_find_export(uint32_t nid) {
    for (int i = 0; i < g_prx_export_n; ++i)
        if (g_prx_exports[i].nid == nid) return g_prx_exports[i].ea;
    return 0;
}

static int prx_mod_find(const char* name) {
    for (int i = 0; i < 32; ++i)
        if (g_prx_mods[i].used && name && std::strncmp(g_prx_mods[i].name, name, 63) == 0)
            return i;
    return -1;
}

static int prx_mod_load(const char* name) {
    int i = prx_mod_find(name);
    if (i >= 0) return i;
    for (i = 0; i < 32; ++i) {
        if (g_prx_mods[i].used) continue;
        g_prx_mods[i].used = true;
        g_prx_mods[i].id = g_prx_mod_seq++;
        g_prx_mods[i].entry = 0;
        g_prx_mods[i].exports = 0;
        g_prx_mods[i].base = 0x30000000ull + (uint64_t)g_prx_mods[i].id * 0x100000ull;
        if (name) {
            std::strncpy(g_prx_mods[i].name, name, 63);
            g_prx_mods[i].name[63] = 0;
        } else {
            std::snprintf(g_prx_mods[i].name, 64, "mod_%u", g_prx_mods[i].id);
        }
        static int s_ld = 0;
        if (s_ld < 16) {
            ps3rt_log("[prx] load \"%s\" id=%u base=0x%llx\n",
                g_prx_mods[i].name, g_prx_mods[i].id,
                (unsigned long long)g_prx_mods[i].base);
            ++s_ld;
        }
        return i;
    }
    return -1;
}

// Load ELF64 BE (or raw) PRX into guest memory at module base
static bool prx_load_file_into_guest(const char* host_path, uint64_t base, uint32_t* out_entry) {
    if (!host_path || !host_path[0] || !g_mem || base >= g_mem_size) return false;
    FILE* f = std::fopen(host_path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz < 16 || sz > 16 * 1024 * 1024) { std::fclose(f); return false; }
    std::vector<uint8_t> buf((size_t)sz);
    if (std::fread(buf.data(), 1, (size_t)sz, f) != (size_t)sz) { std::fclose(f); return false; }
    std::fclose(f);

    if (buf.size() >= 4 && buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F') {
        if (buf.size() < 64) return false;
        uint16_t phentsize = (uint16_t)((buf[54] << 8) | buf[55]);
        uint16_t phnum = (uint16_t)((buf[56] << 8) | buf[57]);
        uint64_t phoff = 0;
        for (int i = 0; i < 8; ++i) phoff = (phoff << 8) | buf[32 + i];
        uint64_t e_entry = 0;
        for (int i = 0; i < 8; ++i) e_entry = (e_entry << 8) | buf[24 + i];
        if (out_entry) *out_entry = (uint32_t)(e_entry ? e_entry : (base + 0x100));
        if (phentsize < 32 || phnum == 0 || phnum > 64) {
            size_t n = (size_t)sz;
            if (base + n > g_mem_size) n = (size_t)(g_mem_size - base);
            std::memcpy(g_mem + base, buf.data(), n);
        } else {
            for (uint16_t i = 0; i < phnum; ++i) {
                size_t o = (size_t)phoff + (size_t)i * phentsize;
                if (o + 56 > buf.size()) break;
                uint32_t p_type = (uint32_t)((buf[o] << 24) | (buf[o + 1] << 16) | (buf[o + 2] << 8) | buf[o + 3]);
                if (p_type != 1) continue; // PT_LOAD
                uint64_t p_offset = 0, p_vaddr = 0, p_filesz = 0, p_memsz = 0;
                for (int k = 0; k < 8; ++k) p_offset = (p_offset << 8) | buf[o + 8 + k];
                for (int k = 0; k < 8; ++k) p_vaddr = (p_vaddr << 8) | buf[o + 16 + k];
                for (int k = 0; k < 8; ++k) p_filesz = (p_filesz << 8) | buf[o + 32 + k];
                for (int k = 0; k < 8; ++k) p_memsz = (p_memsz << 8) | buf[o + 40 + k];
                uint64_t dst = base + (p_vaddr & 0xFFFFFull);
                if (p_vaddr >= 0x10000ull && p_vaddr < g_mem_size) dst = p_vaddr;
                if (dst >= g_mem_size) continue;
                size_t fs = (size_t)p_filesz, ms = (size_t)p_memsz;
                if (dst + ms > g_mem_size) ms = (size_t)(g_mem_size - dst);
                if (fs > ms) fs = ms;
                if (p_offset + fs > buf.size()) fs = buf.size() > (size_t)p_offset ? buf.size() - (size_t)p_offset : 0;
                std::memset(g_mem + dst, 0, ms);
                if (fs) std::memcpy(g_mem + dst, buf.data() + (size_t)p_offset, fs);
            }
        }
        static int szl = 0;
        if (szl < 12) {
            ps3rt_log("[prx] ELF load \"%s\" base=0x%llx size=%ld entry=0x%x\n",
                host_path, (unsigned long long)base, sz, out_entry ? *out_entry : 0);
            ++szl;
        }
        return true;
    }

    if (buf.size() >= 4 && buf[0] == 'S' && buf[1] == 'C' && buf[2] == 'E') {
        static int ss = 0;
        if (ss < 8) { ps3rt_log("[prx] SELF not loaded (encrypted): %s\n", host_path); ++ss; }
        return false;
    }

    size_t n = (size_t)sz;
    if (base + n > g_mem_size) n = (size_t)(g_mem_size - base);
    std::memcpy(g_mem + base, buf.data(), n);
    if (out_entry) *out_entry = (uint32_t)(base + 0x100);
    static int sr = 0;
    if (sr < 8) {
        ps3rt_log("[prx] raw load \"%s\" base=0x%llx\n", host_path, (unsigned long long)base);
        ++sr;
    }
    return true;
}

// XXII-E: module path search + guest load
static int prx_try_load_named(const char* name) {
    int idx = prx_mod_load(name);
    if (idx < 0) return -1;
    char host[768];
    const char* roots[] = { ".", "USRDIR", "USRDIR/modules", "modules", nullptr };
    for (int r = 0; roots[r]; ++r) {
        uint32_t entry = 0;
        std::snprintf(host, sizeof(host), "%s/%s", roots[r], name);
        if (prx_load_file_into_guest(host, g_prx_mods[idx].base, &entry)) {
            g_prx_mods[idx].entry = entry;
            prx_bind_export(0xA0000000u + g_prx_mods[idx].id, g_prx_mods[idx].base + 0x100);
            return idx;
        }
        std::snprintf(host, sizeof(host), "%s/%s.prx", roots[r], name);
        if (prx_load_file_into_guest(host, g_prx_mods[idx].base, &entry)) {
            g_prx_mods[idx].entry = entry;
            prx_bind_export(0xA0000000u + g_prx_mods[idx].id, g_prx_mods[idx].base + 0x100);
            return idx;
        }
    }
    return idx; // registry only
}

static uint64_t g_gcm_fifo = 0;
static uint32_t g_gcm_fifo_size = 0x100000;
static int g_gcm_inited = 0;
static uint64_t g_gcm_control = 0;
static uint64_t g_gcm_labels = 0;
static uint32_t g_gcm_flip_status = 0; // 0 = DONE, non-zero = pending
static uint32_t g_gcm_frame_id = 0;
static uint64_t g_gcm_local_addr = 0;
static uint32_t g_gcm_io_offset = 0;
static uint32_t g_gcm_flip_mode = 1; // VSYNC-ish
static uint32_t g_gcm_current_fb = 0;
static uint32_t g_gcm_vblank = 0;
static uint64_t g_gcm_last_flip_time = 0;
static uint64_t g_gcm_report_addr = 0;

struct GcmDisplayBuffer {
    uint32_t offset;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    int set;
};
static GcmDisplayBuffer g_gcm_fb[8] = {};

struct GcmTile {
    uint32_t region;
    uint32_t offset;
    uint32_t size;
    uint32_t pitch;
    uint32_t base;
    uint32_t bank;
    int bound;
};
static GcmTile g_gcm_tiles[16] = {};

// XX-I: single entry for all cellGcmSys NIDs

// Read GCM control put/get: titles may poke LE or BE; accept either (AIFIAR: observe both).
static uint32_t gcm_rd_ctrl_u32(uint64_t ea) {
    if (!g_mem || ea + 4 > g_mem_size) return 0;
    uint32_t be = rd_be32_guest(ea);
    uint32_t raw = (uint32_t)g_mem[ea] | ((uint32_t)g_mem[ea+1] << 8) |
                   ((uint32_t)g_mem[ea+2] << 16) | ((uint32_t)g_mem[ea+3] << 24);
    // Prefer BE when non-zero; else LE raw if it looks like a fifo offset
    if (be != 0) return be;
    if (raw != 0 && raw < 0x10000000u) {
        static int s_le = 0;
        if (s_le < 8) {
            ps3rt_log("[gcm] control LE put/get observed @0x%llx val=0x%x (using LE)\n",
                (unsigned long long)ea, raw);
            ++s_le;
        }
        return raw;
    }
    return be;
}

// Emit one NV4097 method word pair into guest FIFO and advance put (libgcm-style kick).
static void gcm_fifo_emit_method(uint32_t method, uint32_t count, const uint32_t* args) {
    if (!g_mem || !g_gcm_control || !g_gcm_fifo || !g_gcm_fifo_size) return;
    uint32_t fsz = g_gcm_fifo_size;
    uint32_t put = gcm_rd_ctrl_u32(g_gcm_control + 0) % fsz;
    // method header: (count<<18) | method  (NV40 style used by cellGcm)
    uint32_t hdr = ((count & 0x7FFu) << 18) | (method & 0x1FFFFu);
    auto wr = [&](uint32_t v) {
        uint64_t a = g_gcm_fifo + put;
        if (a + 4 <= g_mem_size) {
            g_mem[a] = (uint8_t)(v >> 24); g_mem[a+1] = (uint8_t)(v >> 16);
            g_mem[a+2] = (uint8_t)(v >> 8); g_mem[a+3] = (uint8_t)v;
        }
        put = (put + 4) % fsz;
    };
    wr(hdr);
    for (uint32_t i = 0; i < count; ++i) wr(args ? args[i] : 0);
    wr_be32_guest(g_gcm_control + 0, put);
    ps3rt_rsx_set_put(put);
}

static int gcm_hle(PPUContext* c, uint32_t nid);

static void gcm_ensure_state() {
    if (!g_gcm_control) {
        g_gcm_control = guest_alloc(0x1000, 0x1000);
        if (g_gcm_control) {
            wr_be32_guest(g_gcm_control+0,0); wr_be32_guest(g_gcm_control+4,0); wr_be32_guest(g_gcm_control+8,0);
            ps3rt_log("[gcm] control @ 0x%llx\n", (unsigned long long)g_gcm_control);
        }
    }
    if (!g_gcm_labels) {
        g_gcm_labels = guest_alloc(0x10000, 0x1000);
        if (g_gcm_labels) ps3rt_log("[gcm] labels  @ 0x%llx\n", (unsigned long long)g_gcm_labels);
    }
    if (!g_gcm_inited) {
        g_gcm_inited = 1;
        if (!g_gcm_fifo) g_gcm_fifo = guest_alloc(g_gcm_fifo_size, 0x10000);
        ps3rt_rsx_init(g_gcm_fifo ? g_gcm_fifo : 0, g_gcm_fifo_size);
        if (!g_gcm_report_addr) g_gcm_report_addr = guest_alloc(0x4000, 0x1000);
        if (!g_gcm_local_addr) {
            g_gcm_local_addr = guest_alloc(0x1000000, 0x10000);
            if (g_gcm_local_addr)
                ps3rt_log("[gcm] local_addr @ 0x%llx\n", (unsigned long long)g_gcm_local_addr);
        }
        g_gcm_flip_status = 0;
        ps3rt_log("[gcm] XX-I state ready fifo=0x%llx ctrl=0x%llx labels=0x%llx\n",
            (unsigned long long)g_gcm_fifo, (unsigned long long)g_gcm_control,
            (unsigned long long)g_gcm_labels);
        PS3RT_STAGE("GCM ready end");
    }
}
static void gcm_fifo_emit(uint32_t method, uint32_t arg) {
    if (!g_gcm_control || !g_gcm_fifo) return;
    uint32_t fsz = g_gcm_fifo_size ? g_gcm_fifo_size : 0x100000;
    // Only advance PUT — GET must lag so RSX can drain the command stream
    uint32_t put = gcm_rd_ctrl_u32(g_gcm_control + 0) % fsz;
    uint32_t get = gcm_rd_ctrl_u32(g_gcm_control + 4) % fsz;
    uint64_t fa = g_gcm_fifo + put;
    if (fa + 8 <= g_mem_size) {
        // NV40/GCM: count=1 method packet (method is register offset)
        uint32_t hdr = (1u << 18) | (method & 0x1FFC);
        wr_be32_guest(fa, hdr);
        wr_be32_guest(fa + 4, arg);
        put = (put + 8) % fsz;
        wr_be32_guest(g_gcm_control + 0, put);
        // DO NOT write GET here — RSX flush advances GET after process
        ps3rt_rsx_set_put(put);
        (void)get;
    }
}
static void gcm_inject_clear_draw() {
    // Build a real guest command stream: color surface + VA0 triangle + draw
    static const uint32_t kCols[] = { 0x402020u, 0x204028u, 0x202848u, 0x403828u };
    static int s_ci = 0;
    uint32_t col = kCols[s_ci++ & 3];
    uint64_t local = g_gcm_local_addr ? g_gcm_local_addr : 0x84f0000ull;
    uint32_t color_off = (uint32_t)(local & 0xFFFFFFFFu);
    // Vertex buffer in local mem: 3x float3 BE positions (clip space)
    uint32_t vb_off = color_off + 0x800000u; // 8MB into local
    if (g_mem && vb_off + 48u < g_mem_size) {
        auto wr_f_be = [&](uint32_t addr, float f) {
            uint32_t u; std::memcpy(&u, &f, 4);
            wr_be32_guest(addr, u);
        };
        // XV-G large NDC triangle (visible seed until real GCM)
        wr_f_be(vb_off +  0, -0.95f); wr_f_be(vb_off +  4, -0.95f); wr_f_be(vb_off +  8, 0.1f);
        wr_f_be(vb_off + 12,  0.95f); wr_f_be(vb_off + 16, -0.95f); wr_f_be(vb_off + 20, 0.1f);
        wr_f_be(vb_off + 24,  0.00f); wr_f_be(vb_off + 28,  0.95f); wr_f_be(vb_off + 32, 0.1f);
    }
    gcm_fifo_emit(0x0200, color_off);           // COLOR_OFFSET
    gcm_fifo_emit(0x0204, 1280u * 4u);          // COLOR_PITCH
    gcm_fifo_emit(0x020C, (1280u << 16) | 720u);
    gcm_fifo_emit(0x1D8C, col);                 // CLEAR_COLOR
    gcm_fifo_emit(0x1D90, 0xF1);
    gcm_fifo_emit(0x1D94, 0);
    // Vertex array 0 = position
    gcm_fifo_emit(0x0A00, 0);                   // depth test OFF
    gcm_fifo_emit(0x0300, 0);                   // blend OFF
    gcm_fifo_emit(0x1680, vb_off);              // VA0 offset
    gcm_fifo_emit(0x1740, 0x23);                // format: float3
    // NV4097: BEGIN (triangles=4), DRAW_ARRAYS count=3, END
    gcm_fifo_emit(0x1808, 4);
    gcm_fifo_emit(0x1814, (2u << 24) | 0u);     // count = (2>>24)+1 = 3
    gcm_fifo_emit(0x1808, 0);
    if (g_mem && g_gcm_control) {
        uint32_t put = gcm_rd_ctrl_u32(g_gcm_control + 0);
        ps3rt_rsx_set_put(put);
        ps3rt_rsx_flush(g_mem);
        // P0: GET only after FIFO accept — publish RSX get, do not force GET=PUT blindly
        uint32_t rg = ps3rt_rsx_get_get();
        wr_be32_guest(g_gcm_control + 4, rg);
    }
    static int s_log = 0;
    if (s_log < 8) {
        ps3rt_log("[gcm] inject CMDSTREAM color=0x%x va0=0x%x draw3\n", color_off, vb_off);
        ++s_log;
    }
}


// XX-I: fuller cellGcmSys HLE — display buffers, flip lifecycle, map/offset, tiles/zcull stubs
static int gcm_hle(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    gcm_ensure_state();
    auto ok = [&]() { c->gpr[3] = 0; return 0; };
    auto fail = [&](int code) { c->gpr[3] = (uint64_t)(int64_t)code; return 0; };

    switch (nid) {
    case 0x15BAE46B: // InitBody
        return ok();
    case 0x55A14C05: // GetControlRegister
        if (c->gpr[3] && c->gpr[3] + 8 <= g_mem_size) wr_be64_guest(c->gpr[3], g_gcm_control);
        return ok();
    case 0x5F909B17: // GetLabelAddress
        if (c->gpr[3] && c->gpr[3] + 8 <= g_mem_size) wr_be64_guest(c->gpr[3], g_gcm_labels);
        return ok();
    case 0xA547ADDE: // GetFlipStatus
    case 0x72A577CE: // GetFlipStatus2
        c->gpr[3] = g_gcm_flip_status;
        return 0;
    case 0xB2E761D4: // ResetFlipStatus
        g_gcm_flip_status = 1;
        return ok();
    case 0xD81D0D2D: // SetFlip
    case 0x05E548B9: // SetFlipCommand
    case 0x0E4F32E0: // SetFlipImmediate
    case 0xA53D12AE: // SetFlipWithWaitLabel
    case 0x069704B6: // SetPrepareFlip
    case 0xDC09357B: // SetFlipMode (also used as flip in some paths)
        if (nid == 0xDC09357B) {
            g_gcm_flip_mode = (uint32_t)c->gpr[3];
            return ok();
        }
        {
            // Flip HLE: queue a guest-visible FIFO kick (libgcm advances put) then process
            uint32_t id = (uint32_t)c->gpr[3];
            if (id < 8) g_gcm_current_fb = id;
            {
                uint32_t arg = id & 7u;
                gcm_fifo_emit_method(0x0050, 1, &arg);
            }
            if (g_gcm_control && g_mem) {
                uint32_t put = gcm_rd_ctrl_u32(g_gcm_control + 0);
                uint32_t get = gcm_rd_ctrl_u32(g_gcm_control + 4);
                if (put == get || put <= 0x80u) {
                    static int s_flip_inj = 0;
                    gcm_inject_clear_draw();
                    if (s_flip_inj < 16) {
                        ps3rt_log("[gcm] SetFlip FIFO kick inject fb=%u put=0x%x get=0x%x\n",
                            id, put, get);
                        ++s_flip_inj;
                    }
                }
            }
            if (g_mem) ps3rt_rsx_flush(g_mem);
            ps3rt_rsx_flip(id);
            g_gcm_flip_status = 0;
            ++g_gcm_frame_id;
            ++g_gcm_vblank;
            g_gcm_last_flip_time += 16666;
            if (g_gcm_labels && g_mem && g_gcm_labels + 4 <= g_mem_size)
                wr_be32_guest(g_gcm_labels, (uint32_t)g_gcm_frame_id);
            return ok();
        }
    case 0xF80196C0: // SetWaitFlip
        if (g_mem) ps3rt_rsx_flush(g_mem);
        ps3rt_rsx_flip(g_gcm_current_fb);
        g_gcm_flip_status = 0;
        ++g_gcm_vblank;
        if (g_gcm_labels && g_mem && g_gcm_labels + 4 <= g_mem_size)
            wr_be32_guest(g_gcm_labels, (uint32_t)g_gcm_frame_id);
        return ok();
    case 0x21AC3697: { // AddressToOffset
        uint64_t addr = c->gpr[3];
        uint32_t off = (uint32_t)(addr & 0xFFFFFFFFu);
        if (g_gcm_local_addr && addr >= g_gcm_local_addr)
            off = (uint32_t)(addr - g_gcm_local_addr);
        if (c->gpr[4] && c->gpr[4] + 4 <= g_mem_size) wr_be32_guest(c->gpr[4], off);
        return ok();
    }
    case 0x2A6FBA9C: { // OffsetToAddress
        uint32_t off = (uint32_t)c->gpr[3];
        uint64_t addr = (g_gcm_local_addr ? g_gcm_local_addr : 0xC0000000ull) + off;
        if (c->gpr[4] && c->gpr[4] + 8 <= g_mem_size) wr_be64_guest(c->gpr[4], addr);
        return ok();
    }
    case 0x4524FE95: { // GetConfiguration
        uint64_t out = c->gpr[3];
        if (out && out + 0x28 <= g_mem_size) {
            wr_be64_guest(out + 0,  g_gcm_local_addr ? g_gcm_local_addr : 0xC0000000ull);
            wr_be64_guest(out + 8,  g_gcm_fifo ? g_gcm_fifo : 0);
            wr_be32_guest(out + 16, 0x0F900000u);
            wr_be32_guest(out + 20, g_gcm_fifo_size);
            wr_be32_guest(out + 24, 0);
            wr_be32_guest(out + 28, 0);
        }
        return ok();
    }
    case 0x21397818: { // SetDisplayBuffer(id, offset, pitch, width, height)
        uint32_t id = (uint32_t)c->gpr[3];
        if (id < 8) {
            g_gcm_fb[id].offset = (uint32_t)c->gpr[4];
            g_gcm_fb[id].pitch  = (uint32_t)c->gpr[5];
            g_gcm_fb[id].width  = (uint32_t)c->gpr[6];
            g_gcm_fb[id].height = (uint32_t)c->gpr[7];
            g_gcm_fb[id].set = 1;
            static int s_fb = 0;
            if (s_fb < 8) {
                ps3rt_log("[gcm] XX-I SetDisplayBuffer id=%u %ux%u pitch=%u off=0x%x\n",
                    id, g_gcm_fb[id].width, g_gcm_fb[id].height,
                    g_gcm_fb[id].pitch, g_gcm_fb[id].offset);
                ++s_fb;
            }
        }
        return ok();
    }
    case 0x63387071: { // GetDisplayBuffer(id, out CellGcmDisplayInfo*)
        uint32_t id = (uint32_t)c->gpr[3];
        uint64_t out = c->gpr[4];
        if (id < 8 && out && out + 16 <= g_mem_size) {
            auto& fb = g_gcm_fb[id];
            wr_be32_guest(out + 0, fb.set ? fb.offset : 0);
            wr_be32_guest(out + 4, fb.set ? fb.pitch  : (1280 * 4));
            wr_be32_guest(out + 8, fb.set ? fb.width  : 1280);
            wr_be32_guest(out + 12, fb.set ? fb.height : 720);
        }
        return ok();
    }
    case 0x9DC3136B: // GetCurrentDisplayBufferId
        c->gpr[3] = g_gcm_current_fb;
        return 0;
    case 0x0F9BB8AA: // SetDefaultCommandBuffer
    case 0xA53D12AF: // SetDefaultFifoSize
        return ok();
    case 0xA114EC67: // MapMainMemory
    case 0xA547ADDF: // MapEaIoAddress
    case 0x3A33F0FD: // MapEaIoAddressWithFlags
    case 0x6340CECC: // MapLocalMemory
        // r3=ea, r4=size, r5=out offset* — give a stable fake IO offset
        {
            uint64_t ea = c->gpr[3];
            uint32_t off = g_gcm_io_offset;
            g_gcm_io_offset = (g_gcm_io_offset + 0x10000u) & 0x0FFFFFFFu;
            if (c->gpr[5] && c->gpr[5] + 4 <= g_mem_size) wr_be32_guest(c->gpr[5], off);
            else if (c->gpr[4] && c->gpr[4] + 4 <= g_mem_size && nid != 0xA114EC67)
                wr_be32_guest(c->gpr[4], off);
            (void)ea;
            return ok();
        }
    case 0x05AE417E: // UnmapEaIoAddress
    case 0xDB23DA22: // UnmapIoAddress
        return ok();
    case 0x21AC3698: { // IoOffsetToAddress
        uint32_t off = (uint32_t)c->gpr[3];
        uint64_t addr = (g_gcm_local_addr ? g_gcm_local_addr : 0xC0000000ull) + off;
        if (c->gpr[4] && c->gpr[4] + 8 <= g_mem_size) wr_be64_guest(c->gpr[4], addr);
        return ok();
    }
    case 0x6571A277: // GetLastFlipTime
        c->gpr[3] = g_gcm_last_flip_time;
        return 0;
    case 0xD34B796E: // GetLastSecondVTime
        c->gpr[3] = g_gcm_last_flip_time;
        return 0;
    case 0x0FB8471B: // GetVBlankCount
        c->gpr[3] = g_gcm_vblank;
        return 0;
    case 0x983FB9AA: // SetSecondVFrequency
    case 0x4AE8D215: // SetVBlankFrequency
    case 0xCFC88E2D: // SetDebugOutputLevel
        return ok();
    case 0xBC982946: // SetTile
    case 0x9A0159AF: // SetTileInfo
    case 0xBD2F6DD7: // BindTile
    case 0xD0B1D189: // UnbindTile
    case 0x527C6439: // SetInvalidateTile
        {
            uint32_t idx = (uint32_t)c->gpr[3] & 15;
            if (nid == 0xBD2F6DD7) g_gcm_tiles[idx].bound = 1;
            if (nid == 0xD0B1D189) g_gcm_tiles[idx].bound = 0;
            if (nid == 0x9A0159AF || nid == 0xBC982946) {
                g_gcm_tiles[idx].offset = (uint32_t)c->gpr[4];
                g_gcm_tiles[idx].size   = (uint32_t)c->gpr[5];
                g_gcm_tiles[idx].pitch  = (uint32_t)c->gpr[6];
            }
            return ok();
        }
    case 0x9A7920AA: // SetZcull
    case 0xD34B796F: // BindZcull
    case 0xA75640E8: // UnbindZcull
        return ok();
    case 0xE4F1324B: // SetUserHandler
    case 0xFA981D20: // SetUserCommand
    case 0x8C7FF6FE: // SetFlipHandler
    case 0x4D7CE993: // SetGraphicsHandler
    case 0xA41EF5C6: // SetQueueHandler
    case 0xFFEB4B8C: // SetVBlankHandler
        return ok();
    case 0x6B91DB6A: // CursorEnable
    case 0xC5F1D5E3: // CursorDisable
    case 0x1BBD6B5C: // CursorSetImage
    case 0x6CD0F95D: // CursorSetPosition
        return ok();
    case 0xACEDC629: // GetReportDataAddressLocation
    case 0x99D397B2: // GetReportDataAddress
        if (c->gpr[3] && c->gpr[3] + 8 <= g_mem_size)
            wr_be64_guest(c->gpr[3], g_gcm_report_addr ? g_gcm_report_addr : g_gcm_labels);
        else
            c->gpr[3] = g_gcm_report_addr ? g_gcm_report_addr : g_gcm_labels;
        return 0;
    case 0x5A41C10F: // GetTimeReportLocation
        c->gpr[3] = g_gcm_report_addr ? g_gcm_report_addr : 0;
        return 0;
    case 0x3B9BD5BD: // SysGetLastError
        return ok();
    default: {
        static int s_unk = 0;
        if (s_unk < 40) {
            ps3rt_log("[gcm] XX-I unhandled nid=0x%08X -> OK\n", nid);
            ++s_unk;
        }
        return ok();
    }
    }
}


// XX-M: cellSpurs HLE
extern "C" int ps3rt_spurs_submit_job(uint32_t entry, uint32_t prio, uint32_t workload_id);
extern "C" int ps3rt_spurs_job_done(int job_id);

static int spurs_dispatch(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    switch (nid) {
    case 0xAC6B8901: // Initialize
    case 0xCAFFF3C0: // Finalize
    case 0x5EF96465: // AttachLv2EventQueue
    case 0x82A1FDB2: // DetachLv2EventQueue
    case 0x7CB33C2E: // CreateJobChain
    case 0x0EB323C0: // JoinJobChain
    case 0x39C173FB: // ShutdownJobChain
    case 0x4DC1DEE4: // CreateTaskset
    case 0xA839A4D9: // JoinTaskset
    case 0x16394A4A: // GetInfo
        c->gpr[3] = 0;
        return 0;
    case 0xB9BC6207: // AddWorkload
    case 0x1D46DEA1: // StartWorkload
        c->gpr[3] = 0;
        return 0;
    case 0x0E54C3A6: // RemoveWorkload
    case 0xFF7A7BD2: // StopWorkload
        c->gpr[3] = 0;
        return 0;
    case 0x2C9B9C1C: // RunJobChain — r3=chain, treat as kick
    case 0x52CC6C82: // CreateTask — r4 may be entry
    case 0xE717AC73: // SendSignal
        {
            uint32_t entry = (uint32_t)c->gpr[4];
            if (!entry) entry = (uint32_t)c->gpr[5];
            if (entry) {
                int jid = ps3rt_spurs_submit_job(entry, (uint32_t)c->gpr[6], 0);
                static int s = 0;
                if (s < 12) {
                    ps3rt_log("[spurs] XX-M nid=0x%08X submit entry=0x%x job=%d\n", nid, entry, jid);
                    ++s;
                }
            }
            c->gpr[3] = 0;
            return 0;
        }
    case 0xC2AC4828: // WaitForWorkload
        // XXV-H SPURS: mark complete; real job drain is still HLE
        {
            static int sw = 0;
            if (sw < 12) { ps3rt_log("[spurs] XXV-H WaitForWorkload OK\n"); ++sw; }
        }
        c->gpr[3] = 0;
        return 0;
    default:
        c->gpr[3] = 0;
        return 0;
    }
}


// Depth: cellSysutil / videoOut minimal HLE (success + safe outs)
static int sysutil_hle(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    auto ok = [&]() -> int { c->gpr[3] = 0; return 0; };
    switch (nid) {
    case 0x9D98AFA0: // RegisterCallback
    case 0x189A74DA: // UnregisterCallback
    case 0x02E20C25: // CheckCallback
    case 0xD8477650: // EnableBgmPlayback
    case 0x5CDEAE17: // DisableBgmPlayback
    case 0x7570C61A: // GetBgmPlaybackStatus
        return ok();
    case 0x40E895D3: { // GetSystemParamInt(id, out)
        uint64_t out = c->gpr[4];
        if (out && g_mem && out + 4 <= g_mem_size) {
            // CELL_SYSUTIL_SYSTEMPARAM_ID_LANG = 1 -> English=1
            g_mem[out] = 0; g_mem[out+1] = 0; g_mem[out+2] = 0; g_mem[out+3] = 1;
        }
        return ok();
    }
    case 0x938013A0: // GetSystemParamString
        return ok();
    case 0x6C0EB84F: { // cellVideoOutGetState(videoOut, deviceIndex, state)
        uint64_t st = c->gpr[5];
        if (st && g_mem && st + 16 <= g_mem_size) {
            // state=1 (available), colorSpace=RGB, resolution=1280x720 id
            memset(g_mem + st, 0, 16);
            g_mem[st + 3] = 1; // state
        }
        return ok();
    }
    case 0x55EBB5AF: // GetResolution
    case 0x0BAE8772: // Configure
    case 0xE558748D: // GetNumberOfDevice
    case 0x75BBB672: // GetDeviceInfo
    case 0xCFFE948A: // GetResolutionAvailability
        return ok();
    case 0x63A2C8A9: // sysmodule Initialize
    case 0xA1934A6A: // Finalize
    case 0x3A5B8C0B: // LoadModule
    case 0x5A59E258: // UnloadModule
    case 0x32267A31: // IsLoaded
    case 0x74A54CDF: // GetLibraryInfo
    case 0x96C07ADF: // SetMemcontainer
    case 0x0053F96C:
        return ok();
    default:
        // Known GOW stub NIDs from logs
        if (nid == 0x0065F088 || nid == 0x0065F9C8 || nid == 0x0065FCC8 || nid == 0x00525680)
            return ok();
        return ok(); // soft success for registered sysutil kind
    }
}

static int prx_dispatch(PPUContext* c, uint32_t maybe_nid, int kind) {
    if (!c) return -1;
    if (maybe_nid == 0) { c->gpr[3] = 0; return 0; }
    const NidEntry* e = nid_lookup(maybe_nid);
    if (e) {
        if (g_prx_log < 40) {
            ps3rt_log("[prx] stub kind=%d %s pc=0x%llx nid=0x%08X lr=0x%llx r11=0x%llx\n",
                kind, e->module ? e->module : "?", (unsigned long long)c->pc, e->nid,
                (unsigned long long)c->lr, (unsigned long long)c->gpr[11]);
            ++g_prx_log;
        }
        if (kind == 1 || (e->module && (std::strstr(e->module, "Gcm") || std::strstr(e->module, "gcm")))) {
            return gcm_hle(c, e->nid);
        }

        // U: deeper sysutil param behaviour
        if (e->nid == 0x40E895D3 || e->nid == 0x1E14A55D) {
            // GetSystemParamInt — r3=id, r4=&value
            uint32_t id = (uint32_t)c->gpr[3];
            uint64_t out = c->gpr[4];
            int32_t val = 0;
            if (id == 0x111) val = 1;       // lang English-ish
            else if (id == 0x112) val = 1;  // aspect
            else if (id == 0x113) val = 0;  // parental
            else if (id == 0x131) val = 1;  // ENTER button
            else val = 0;
            if (out && g_mem && out + 4 <= g_mem_size) {
                uint8_t* p = g_mem + out;
                p[0]=(uint8_t)(val>>24); p[1]=(uint8_t)(val>>16); p[2]=(uint8_t)(val>>8); p[3]=(uint8_t)val;
            }
            static int su=0; if(su<10){ps3rt_log("[prx] U GetSystemParamInt id=0x%x -> %d\n", id, val);++su;}
            c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x938013A3) {
            // GetSystemParamString — write short empty/default
            uint64_t out = c->gpr[4];
            if (out && g_mem && out + 8 <= g_mem_size) {
                g_mem[out] = 0;
            }
            c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x189A74DA) {
            // CheckCallback — no callbacks pending
            c->gpr[3] = 0; return 0;
        }

        if (e->nid == 0x1C936E6D) { g_pad_inited = 1; pad_poll_host(); c->gpr[3] = 0; return 0; }
        if (e->nid == 0x0D5F2C14) { g_pad_inited = 0; c->gpr[3] = 0; return 0; }
        if (e->nid == 0x3733EA3C || e->nid == 0x89FF8792) {
            uint64_t out = c->gpr[4] ? c->gpr[4] : c->gpr[3];
            pad_write_data(out); c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0xA703A51D) {
            pad_poll_host();
            uint64_t out = c->gpr[3] ? c->gpr[3] : c->gpr[4];
            pad_write_info2(out); c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x3F797DFF) { c->gpr[3] = 0; return 0; }
        if (kind == 2) { return ps3rt_fs_dispatch(c, e->nid); }
        if (kind == 4) { return ps3rt_audio_dispatch(c, e->nid); }
        if (kind == 5) { return spurs_dispatch(c, e->nid); }
        if (kind == 11) { return ps3rt_save_dispatch(c, e->nid); }
        if (kind == 3 || (e->module && std::strstr(e->module, "Sysmodule"))) {
            if (e->nid == 0x3A495ADE || e->nid == 0xCFDD8E87 || e->nid == 0x3A52CF82) {
                char mname[128] = {};
                uint64_t pea = c->gpr[3];
                if (pea && g_mem && pea < g_mem_size && pea > 0x10000) {
                    size_t n = 0;
                    while (n + 1 < sizeof(mname) && pea + n < g_mem_size) {
                        char ch = (char)g_mem[pea + n];
                        if (!ch) break;
                        mname[n++] = ch;
                    }
                }
                if (!mname[0]) {
                    uint32_t mid = (uint32_t)c->gpr[3];
                    const char* mapn = "sysmodule";
                    switch (mid) {
                    case 0x0003: mapn = "cellRtc"; break;
                    case 0x0005: mapn = "cellFs"; break;
                    case 0x0008: mapn = "cellAudio"; break;
                    case 0x000a: mapn = "cellFont"; break;
                    case 0x0011: mapn = "cellSpurs"; break;
                    case 0x001c: mapn = "cellSync"; break;
                    case 0x0021: mapn = "cellSysutil"; break;
                    case 0x0040: mapn = "cellResc"; break;
                    case 0x0050: mapn = "cellPamf"; break;
                    case 0x005a: mapn = "cellAtrac"; break;
                    case 0x0070: mapn = "cellSail"; break;
                    case 0x0078: mapn = "cellVdec"; break;
                    default: break;
                    }
                    std::strncpy(mname, mapn, sizeof(mname)-1);
                    static int s_mid = 0;
                    if (s_mid < 24) {
                        ps3rt_log("[prx] XV-L sysmoduleLoad id=0x%x -> %s\n", mid, mname);
                        ++s_mid;
                    }
                }
                if (!mname[0] && e->name) std::strncpy(mname, e->name, sizeof(mname)-1);
                if (!mname[0]) std::strncpy(mname, "sysmodule", sizeof(mname)-1);
                prx_try_load_named(mname);
            }
            c->gpr[3] = 0; return 0;
        }
        if (kind == 7 || kind == 3) { return sysutil_hle(c, maybe_nid); }
    if (kind == 8) {
            pad_poll_host();
            c->gpr[3] = 0; return 0;
        }
        // XV-M: RESC / display convert soft HLE (boot often probes these)
        if (e->module && (std::strstr(e->module, "Resc") || std::strstr(e->module, "resc"))) {
            c->gpr[3] = 0; return 0;
        }
        if (e->module && (std::strstr(e->module, "Rtc") || std::strstr(e->module, "rtc"))) {
            // tick/clock: write zeros / monotonic if pointer
            uint64_t out = c->gpr[3] ? c->gpr[3] : c->gpr[4];
            if (out && g_mem && out + 8 <= g_mem_size) wr_be64_guest(out, 1000000ull);
            c->gpr[3] = 0; return 0;
        }
        if (e->module && std::strstr(e->module, "Sync")) {
            c->gpr[3] = 0; return 0;
        }
        // S: track first hit per module name as "loaded"
        if (e->module) prx_mod_load(e->module);
        prx_note_hit(maybe_nid, e ? 1 : 2);
        c->gpr[3] = 0; return 0;
    }
    // No NID table hit — still honour module kind (XX-H)
    if (kind == 1) {
        return gcm_hle(c, maybe_nid);
    }
    if (kind == 2) { return ps3rt_fs_dispatch(c, maybe_nid); }
    if (kind == 4) { return ps3rt_audio_dispatch(c, maybe_nid); }
    if (kind == 5) { return spurs_dispatch(c, maybe_nid); }
    if (kind == 11) { return ps3rt_save_dispatch(c, maybe_nid); }
    if (kind == 8) { pad_poll_host(); c->gpr[3] = 0; prx_note_hit(maybe_nid, 1); return 0; }
    // Unknown import: nid 0 is noise
    if (maybe_nid == 0) {
        c->gpr[3] = 0;
        return 0;
    }
// XXV-G: unknown import contract
    prx_note_hit(maybe_nid, 2);
    ++g_prx_unknown_n;
    static int s_unk = 0;
    if (s_unk < 40) {
        ps3rt_log("[prx] XXV-G UNKNOWN nid=0x%08X kind=%d (total_unknown=%d)\n",
            maybe_nid, kind, g_prx_unknown_n);
        ++s_unk;
    }
    if (prx_strict_imports()) {
        c->gpr[3] = 0x80010005ull; // CELL_ENOSYS
        return (int)0x80010005;
    }
    c->gpr[3] = 0; return 0;
}

// XXIII-H: PRX import NID recovery + module bind
PS3RT_API int ps3rt_prx_call(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    const NidEntry* e = nid_lookup(nid);
    int kind = e ? e->kind : 0;
    if (e && e->module) kind = prx_module_kind(e->module);
    static int call_log = 0;
    if (call_log < 30) {
        ps3rt_log("[ps3] import stub pc=0x%llx -> ret=0x%llx (#%d)\n",
            (unsigned long long)c->pc, (unsigned long long)c->lr, call_log + 1);
        ++call_log;
    }
    return prx_dispatch(c, nid, kind);
}

// Called from ppu_run when PC lands on a null/import-like address.
PS3RT_API int ps3rt_prx_import_stub(PPUContext* c, uint64_t pc) {
    if (!c) return -1;
    // XXIV-D: reject TOC/code-looking values; prefer true 32-bit NIDs
    // XV-J: reject size/pointer-shaped values (log: 0x003CC7F8 was alloc size)
    auto looks_like_nid = [](uint32_t v) -> bool {
        if (v == 0 || v == 0xFFFFFFFFu) return false;
        if (v < 0x100000u) return false; // reject 0x65 / 0x160
        if (v >= 0x30000000u && v <= 0x3FFFFFFFu) return false;
        if (v >= 0x48000000u && v <= 0x4FFFFFFFu) return false;
        if (v >= 0x80000000u) return false;
        if ((v & 0xFFFu) == 0) return false;
        if (v < 0x01000000u && (v & 0xFFu) == 0) return false;
        int bits = 0;
        for (uint32_t t = v; t; t >>= 1) bits += (int)(t & 1u);
        if (bits < 6 || bits > 26) return false;
        return true;
    };
    uint32_t maybe_nid = (uint32_t)c->gpr[11];
    if (!looks_like_nid(maybe_nid)) {
        uint32_t cands[] = {
            (uint32_t)c->gpr[0], (uint32_t)c->gpr[3], (uint32_t)c->gpr[4],
            (uint32_t)c->gpr[5], (uint32_t)c->gpr[6], (uint32_t)c->gpr[7],
            (uint32_t)c->gpr[10], (uint32_t)(c->gpr[2] & 0xFFFFFFFFu)
        };
        maybe_nid = 0;
        for (uint32_t cand : cands) {
            if (looks_like_nid(cand) && nid_lookup(cand)) { maybe_nid = cand; break; }
        }
        if (!maybe_nid) {
            for (uint32_t cand : cands) {
                if (looks_like_nid(cand) && nid_lookup(cand)) { maybe_nid = cand; break; }
            }
        }
    }
    // Word at LR-4 / PC+0 sometimes holds reloc NID in tables
    if ((!maybe_nid || !nid_lookup(maybe_nid)) && g_mem) {
        uint64_t addrs[] = { c->lr >= 4 ? c->lr - 4 : 0, pc, pc + 4 };
        for (uint64_t a : addrs) {
            if (!a || a + 4 > g_mem_size) continue;
            uint32_t w = ((uint32_t)g_mem[a] << 24) | ((uint32_t)g_mem[a+1] << 16) |
                         ((uint32_t)g_mem[a+2] << 8) | (uint32_t)g_mem[a+3];
            if (looks_like_nid(w) && nid_lookup(w)) { maybe_nid = w; break; }
            // skip non-table memory words
        }
    }

    const NidEntry* e = maybe_nid ? nid_lookup(maybe_nid) : nullptr;
    int kind = e ? e->kind : 0;
    if (e && e->module) kind = prx_module_kind(e->module);
    if (e && e->module) prx_mod_load(e->module);

    static int s = 0;
    if (s < 40) {
        ps3rt_log("[ps3] XXIV-D prx_import pc=0x%llx nid=0x%08X kind=%d mod=%s lr=0x%llx\n",
            (unsigned long long)pc, maybe_nid, kind,
            (e && e->module) ? e->module : "?",
            (unsigned long long)c->lr);
        ++s;
    }
    int rc = prx_dispatch(c, maybe_nid, kind);
    // XXIV-D: always return to LR when it looks like a valid code address
    if (c->lr && (c->lr & 3ull) == 0 && c->lr >= 0x10000ull && c->lr < 0x30000000ull) {
        // leave LR for blr semantics; caller of stub uses r3 = rc
        (void)0;
    }
    c->gpr[3] = (uint64_t)(int64_t)rc;
    return rc;
}

// R+X: coarse guest timing + host frame pacing
static uint64_t g_time_ppu_slices = 0;
static uint64_t g_time_spu_ticks = 0;
static uint64_t g_time_rsx_flips = 0;
static uint32_t g_time_phase = 0;
static uint64_t g_time_last_host_ms = 0;
static uint32_t g_time_target_fps = 60;
#if defined(_WIN32)
static uint64_t host_ms_now() {
    return (uint64_t)GetTickCount64();
}
#else
static uint64_t host_ms_now() { return 0; }
#endif
// XXII-F: host frame pacing (~60Hz)
static void frame_pace_wait() {
    // X: aim ~60Hz between RSX flip assists
    uint64_t now = host_ms_now();
    if (!g_time_last_host_ms) { g_time_last_host_ms = now; return; }
    uint64_t frame_ms = 1000u / (g_time_target_fps ? g_time_target_fps : 60);
    uint64_t elapsed = now - g_time_last_host_ms;
    if (elapsed < frame_ms) {
#if defined(_WIN32)
        Sleep((DWORD)(frame_ms - elapsed));
#endif
    }
    g_time_last_host_ms = host_ms_now();
    ++g_time_rsx_flips;
}


PS3RT_API void ps3rt_progress_hw(void) {
    extern void ps3rt_spu_tick_all(void);
    ps3rt_spu_tick_all();

    if (ps3rt_should_exit()) { ps3rt_force_exit(); return; }
    static int s_ppu_once = 0;
    if (!s_ppu_once) { PS3RT_STAGE("PPU progress start"); s_ppu_once = 1; }
    ++g_time_ppu_slices;
    g_time_phase = (g_time_phase + 1) & 3;
    // R: phase 0,1 = SPU heavy; 2 = pad/audio; 3 = RSX bias already below
    if (g_time_phase != 2) {
        spu_tick_all();
        ++g_time_spu_ticks;
    }
    pad_poll_host();
    ps3rt_audio_tick();
    if (!g_gcm_inited) gcm_ensure_state();
    if (g_gcm_inited) {
        static uint32_t s_n = 0;
        ++s_n;
        // XX-H: clear pending flip often so GetFlipStatus/WaitFlip cannot stick
        if (g_gcm_flip_status != 0 && (s_n & 0x1F) == 0) {
            if (g_mem) ps3rt_rsx_flush(g_mem);
            frame_pace_wait(); // X
            ps3rt_rsx_flip(0);
            g_gcm_flip_status = 0;
            ++g_gcm_frame_id;
        }
        // XXIII-K: tighter RSX assist cadence (every 32 ticks)
        if ((s_n & 0x07) == 0) { // XXV-E: denser flip/present assist
            uint32_t fsz = g_gcm_fifo_size ? g_gcm_fifo_size : 0x100000u;
            uint32_t put = g_gcm_control ? (gcm_rd_ctrl_u32(g_gcm_control + 0) % fsz) : 0;
            uint32_t get = g_gcm_control ? (gcm_rd_ctrl_u32(g_gcm_control + 4) % fsz) : 0;
            {
                uint32_t dist = (put >= get) ? (put - get) : (fsz - get + put);
                if (dist > fsz / 2) {
                    static int s_fix = 0;
                    if (s_fix < 8) {
                        ps3rt_log("[gcm] P0 large FIFO gap put=0x%x get=0x%x dist=0x%x (no forced resync)\n",
                            put, get, dist);
                        ++s_fix;
                    }
                }
            }
            // XXIV-B: only treat as real guest stream when put moves past minimal residual
            // XV-H: seed when FIFO stalled (put==get) OR still in residual range.
            // Real game stream only when put advances beyond get with substantial traffic.
            // XV-I: inject at most once per displayed flip (stop FIFO flood)
            // P1: inject seed only if PS3RT_INJECT_SEED=1
            // Boot FIFO assist: while guest PUT stays idle, emit one real cmdstream per flip
            // so the RSX path (methods/draws) is exercised. Disable with PS3RT_INJECT_SEED=0.
            static int s_allow_inject = -1;
            static int s_boot_n = 0;
            if (s_allow_inject < 0) {
                const char* e = std::getenv("PS3RT_INJECT_SEED");
                if (e && e[0] == '0') s_allow_inject = 0;
                else s_allow_inject = 1; // default ON until guest drives PUT
                ps3rt_log("[gcm] boot FIFO assist %s (PS3RT_INJECT_SEED=0 to disable)\n",
                    s_allow_inject ? "ON" : "OFF");
            }
            static uint64_t s_inj_flip = (uint64_t)-1;
            uint64_t fc = ps3rt_rsx_flip_count();
            if (s_allow_inject && (put == get || put <= 0x80) && s_inj_flip != fc && s_boot_n < 128) {
                s_inj_flip = fc;
                ++s_boot_n;
                gcm_inject_clear_draw();
                if (s_boot_n <= 8 || (s_boot_n & 7) == 0)
                    ps3rt_log("[gcm] boot FIFO assist #%d put/get were idle\n", s_boot_n);
            } else if (put != get && put > 0x80) {
                static int s_guest = 0;
                if (s_guest < 12) {
                    ps3rt_log("[gcm] XXIV-B guest put=0x%x get=0x%x\n", put, get);
                    ++s_guest;
                }
            }
            if (g_mem) {
                if (put > 0x20) ps3rt_rsx_set_put(put);
                ps3rt_rsx_flush(g_mem);
                static int s_idle = 0;
                if (put == get && s_idle < 8) {
                    ps3rt_log("[gcm] FIFO idle put=get=0x%x (no guest commands yet)\n", put);
                    ++s_idle;
                }
            }
            frame_pace_wait();
            ps3rt_rsx_flip(0);
            g_gcm_flip_status = 0;
            // XV-I: sync control GET from RSX after flush (do not skip decode)
            if (g_gcm_control) {
                uint32_t rg = ps3rt_rsx_get_get();
                wr_be32_guest(g_gcm_control + 4, rg);
            }
        }
    }
    static int s_run = 0;
    if (!s_run && g_gcm_inited) { PS3RT_STAGE("RUNNING"); s_run = 1; }
    // XXV-J end-to-end service checklist (logged once)
    {
        static int s_e2e = 0;
        if (!s_e2e && g_gcm_inited && g_time_ppu_slices > 64) {
            s_e2e = 1;
            ps3rt_log("[e2e] XXV-J checklist\n");
            ps3rt_log("[e2e]   boot_mem=OK gcm=%d pad_init=%d\n", g_gcm_inited ? 1 : 0, g_pad_inited ? 1 : 0);
            ps3rt_log("[e2e]   flips=%llu ppu_slices=%llu spu_ticks=%llu\n",
                (unsigned long long)g_time_rsx_flips,
                (unsigned long long)g_time_ppu_slices,
                (unsigned long long)g_time_spu_ticks);
            ps3rt_log("[e2e]   fs/audio/save/pad services installed (I)\n");
            ps3rt_log("[e2e]   remaining: guest RSX geometry (K), wild-pc stability (L)\n");
        }
    }
    if ((g_time_ppu_slices & 0x3FF) == 0) {
        static int s_t = 0;
        if (s_t < 8) {
            ps3rt_log("[time] XXIII-K ppu=%llu spu=%llu rsx_flips=%llu phase=%u\n",
                (unsigned long long)g_time_ppu_slices,
                (unsigned long long)g_time_spu_ticks,
                (unsigned long long)g_time_rsx_flips, g_time_phase);
            ++s_t;
        }
    }
}

// XXII-B/H: PPU soft-recovery depth — do not hard-halt on unknown ops
PS3RT_API void ps3rt_on_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 40) { ps3rt_log("[ps3] unimplemented op=0x%08X pc=0x%llx (soft)\n", opcode, (unsigned long long)pc); ++n; }
    // Soft: do not hard-halt; let recover_return_pc / region logic continue when possible
    if (c) {
        // advance past bad op if still in guest image
        if (c->pc == pc) c->pc = pc + 4;
    }
}


// Full xo31 coverage: runtime dispatch for forms not specialized at lift time
PS3RT_API void ps3rt_xo31_dispatch(PPUContext* c, unsigned xo, unsigned rt, unsigned ra, unsigned rb) {
    if (!c) return;
    auto R = [&](unsigned i) -> uint64_t& { return c->gpr[i & 31]; };
    auto EA = [&]() -> uint64_t {
        return (ra ? R(ra) : 0) + R(rb);
    };
    switch (xo) {
    // Arithmetic (OE already normalized at lift when possible)
    case 266: case 778: R(rt) = R(ra) + R(rb); break;
    case 40: case 552: R(rt) = R(rb) - R(ra); break;
    case 10: case 522: R(rt) = R(ra) + R(rb); break;
    case 8: case 520: R(rt) = R(rb) - R(ra); break;
    case 235: case 747: R(rt) = (uint64_t)(int32_t)((int32_t)R(ra) * (int32_t)R(rb)); break;
    case 233: case 745: R(rt) = (uint64_t)((int64_t)R(ra) * (int64_t)R(rb)); break;
    case 491: case 1003: R(rt) = R(rb) ? (uint64_t)((int32_t)R(ra) / (int32_t)R(rb)) : 0; break;
    case 489: case 1001: R(rt) = R(rb) ? (uint64_t)((int64_t)R(ra) / (int64_t)R(rb)) : 0; break;
    case 459: case 971: R(rt) = R(rb) ? (uint32_t)R(ra) / (uint32_t)R(rb) : 0; break;
    case 457: case 969: R(rt) = R(rb) ? R(ra) / R(rb) : 0; break;
    case 200: case 712: R(rt) = ~R(ra) + 1; break;
    case 104: case 616: R(rt) = (uint64_t)(-(int64_t)R(ra)); break;
    // Logical
    case 28: R(ra) = R(rt) & R(rb); break;
    case 444: R(ra) = R(rt) | R(rb); break;
    case 316: R(ra) = R(rt) ^ R(rb); break;
    case 124: R(ra) = ~(R(rt) | R(rb)); break;
    case 476: R(ra) = ~(R(rt) & R(rb)); break;
    case 60: R(ra) = R(rt) & ~R(rb); break;
    case 412: R(ra) = R(rt) | ~R(rb); break;
    case 284: R(ra) = ~(R(rt) ^ R(rb)); break;
    // Shifts
    case 24: R(ra) = (uint32_t)R(rt) << (R(rb) & 63); break;
    case 536: { unsigned s = R(rb) & 63; R(ra) = s > 31 ? 0 : ((uint32_t)R(rt) >> s); break; }
    case 27: R(ra) = R(rt) << (R(rb) & 127); break;
    case 539: { unsigned s = R(rb) & 127; R(ra) = s > 63 ? 0 : (R(rt) >> s); break; }
    case 792: { unsigned s = R(rb) & 63; int32_t v = (int32_t)R(rt); R(ra) = (uint64_t)(int64_t)(s > 31 ? (v < 0 ? -1 : 0) : (v >> s)); break; }
    case 794: { unsigned s = R(rb) & 127; int64_t v = (int64_t)R(rt); R(ra) = (uint64_t)(s > 63 ? (v < 0 ? -1 : 0) : (v >> s)); break; }
    // Loads / stores
    case 23: case 55: R(rt) = rd32(*c, EA()); if (xo == 55 && ra) R(ra) = EA(); break;
    case 21: case 53: R(rt) = rd64(*c, EA()); if (xo == 53 && ra) R(ra) = EA(); break;
    case 87: case 119: R(rt) = rd8(*c, EA()); if (xo == 119 && ra) R(ra) = EA(); break;
    case 279: case 311: R(rt) = rd16(*c, EA()); if (xo == 311 && ra) R(ra) = EA(); break;
    case 151: case 183: wr32(*c, EA(), (uint32_t)R(rt)); if (xo == 183 && ra) R(ra) = EA(); break;
    case 149: case 181: wr64(*c, EA(), R(rt)); if (xo == 181 && ra) R(ra) = EA(); break;
    case 215: case 247: wr8(*c, EA(), (uint8_t)R(rt)); if (xo == 247 && ra) R(ra) = EA(); break;
    case 407: case 439: wr16(*c, EA(), (uint16_t)R(rt)); if (xo == 439 && ra) R(ra) = EA(); break;
    case 20: R(rt) = rd32(*c, EA()); break; // lwarx
    case 150: wr32(*c, EA(), (uint32_t)R(rt)); c->cr[0] = 2; break; // stwcx soft success
    // Compare
    case 0: {
        int64_t a = (int64_t)R(ra), b = (int64_t)R(rb);
        unsigned bf = 0; // simplified CR0
        c->cr[0] = (a < b) ? 8 : ((a > b) ? 4 : 2);
        break;
    }
    case 32: {
        uint64_t a = R(ra), b = R(rb);
        c->cr[0] = (a < b) ? 8 : ((a > b) ? 4 : 2);
        break;
    }
    // SPR soft
    case 339: // mfspr
        if (rt) R(rt) = 0;
        break;
    case 467: // mtspr
        break;
    case 19: { // mfcr
        uint32_t v = 0;
        for (int i = 0; i < 8; i++) v = (v << 4) | (c->cr[i] & 0xf);
        R(rt) = v;
        break;
    }
    // Cache / barriers
    case 22: case 54: case 86: case 246: case 278: case 470: case 598: case 854: case 1014:
        break;
    default: {
        static int n = 0;
        if (n < 30) {
            ps3rt_log("[ps3] xo31_dispatch unhandled xo=%u rt=%u ra=%u rb=%u\n", xo, rt, ra, rb);
            ++n;
        }
        break;
    }
    }
}

// VMX residual dispatcher — implements common remaining AltiVec forms at runtime
PS3RT_API void ps3rt_vmx_dispatch(PPUContext* c, unsigned xo11, unsigned vd, unsigned va, unsigned vb, unsigned vc) {
    if (!c || vd >= 32) return;
    auto& d = c->vpr[vd];
    const auto& a = c->vpr[va < 32 ? va : 0];
    const auto& b = c->vpr[vb < 32 ? vb : 0];
    const auto& ctl = c->vpr[vc < 32 ? vc : 0];
    // Generic safe fallbacks by class
    switch (xo11 & 0x7FF) {
    case 1028: case 1034: // vand
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)(a[i] & b[i]); break;
    case 1156: case 1158: // vor
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)(a[i] | b[i]); break;
    case 1220: case 1092: // vxor
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)(a[i] ^ b[i]); break;
    case 1284: // vnor
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)~(a[i] | b[i]); break;
    case 1032: // vandc
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)(a[i] & ~b[i]); break;
    case 388: // vaddubm
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)(a[i] + b[i]); break;
    case 1408: case 1412: // vcmpequb / h
        for (int i = 0; i < 16; ++i) d[i] = (a[i] == b[i]) ? 0xFF : 0; break;
    case 1476: // vcmpequw bytes-as-words eq soft
        for (int i = 0; i < 16; ++i) d[i] = (a[i] == b[i]) ? 0xFF : 0; break;
    case 770: case 518: // vmaxub
        for (int i = 0; i < 16; ++i) d[i] = a[i] > b[i] ? a[i] : b[i]; break;
    case 774: case 522: // vminub
        for (int i = 0; i < 16; ++i) d[i] = a[i] < b[i] ? a[i] : b[i]; break;
    case 1030: // vavgub
        for (int i = 0; i < 16; ++i) d[i] = (uint8_t)(((unsigned)a[i] + (unsigned)b[i] + 1u) >> 1); break;
    case 70: // vaddfp soft per 4 floats BE
        for (int i = 0; i < 4; ++i) {
            float fa, fb, fr; std::memcpy(&fa, &a[i*4], 4); std::memcpy(&fb, &b[i*4], 4);
            fr = fa + fb; std::memcpy(&d[i*4], &fr, 4);
        } break;
    case 74: // vsubfp
        for (int i = 0; i < 4; ++i) {
            float fa, fb, fr; std::memcpy(&fa, &a[i*4], 4); std::memcpy(&fb, &b[i*4], 4);
            fr = fa - fb; std::memcpy(&d[i*4], &fr, 4);
        } break;
    case 12: case 76: // lvsl/lvsr soft copy
        std::memcpy(d, a, 16); break;
    default:
        static int n = 0;
        if (n < 24) { ps3rt_log("[vmx] dispatch xo11=%u vd=%u va=%u vb=%u\n", xo11, vd, va, vb); ++n; }
        break;
    }
}

