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
    static int sys_log = 0;
    if (sys_log < 80) { ps3rt_log("[lv2] sc %llu r3=%llx r4=%llx r5=%llx r6=%llx\n", (unsigned long long)num, (unsigned long long)r3, (unsigned long long)r4, (unsigned long long)r5, (unsigned long long)r6); ++sys_log; }
    switch ((uint32_t)num) {
    case 0: case 1: c->halted = true; c->gpr[3] = 0; break;
    case 3:
        ps3rt_request_exit();
        c->halted = true; c->gpr[3] = 0; break;
    case 14: case 15: case 141: case 142:
    case 871: case 872: case 25602: c->gpr[3] = 0; break;
    case 128: {
        static uint32_t qid = 0x1000;
        if (r3 && r3 + 4 <= g_mem_size) wr_be32_guest(r3, ++qid);
        c->gpr[3] = 0; break;
    }
    case 143: { static uint32_t fid = 0x8000; if (r3 && r3+4 <= g_mem_size) wr_be32_guest(r3, ++fid); c->gpr[3] = 0; break; }
    case 348: case 349: case 350: case 352: {
        uint64_t a = guest_alloc(r4 ? r4 : 0x1000, r5 ? r5 : 0x1000);
        if (r3 && r3 + 8 <= g_mem_size) wr_be64_guest(r3, a);
        c->gpr[3] = a ? 0 : (uint64_t)-1; break;
    }
    default: c->gpr[3] = 0; break;
    }
}

static int g_prx_log = 0;

// S: loaded PRX module registry (name + id)
struct PrxMod { bool used; char name[64]; uint32_t id; uint32_t entry; uint32_t exports; uint64_t base; };
// W: simple export bind table (NID -> guest stub addr)
struct PrxExport { uint32_t nid; uint64_t ea; };
static PrxExport g_prx_exports[128];
static int g_prx_export_n = 0;
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

static PrxMod g_prx_mods[32];
static uint32_t g_prx_mod_seq = 1;
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
        g_prx_mods[i].base = 0;
        if (name) {
            std::strncpy(g_prx_mods[i].name, name, 63);
            g_prx_mods[i].name[63] = 0;
        } else {
            std::snprintf(g_prx_mods[i].name, 64, "mod_%u", g_prx_mods[i].id);
        }
        static int s_ld = 0;
        if (s_ld < 16) {
            ps3rt_log("[prx] S load "%s" id=%u\n", g_prx_mods[i].name, g_prx_mods[i].id);
            ++s_ld;
        }
        return i;
    }
    return -1;
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
    uint32_t put = rd_be32_guest(g_gcm_control + 0) % fsz;
    uint64_t fa = g_gcm_fifo + put;
    if (fa + 8 <= g_mem_size) {
        uint32_t hdr = (1u << 18) | (method & 0x1FFC);
        wr_be32_guest(fa, hdr);
        wr_be32_guest(fa + 4, arg);
        put = (put + 8) % fsz;
        wr_be32_guest(g_gcm_control + 0, put);
        wr_be32_guest(g_gcm_control + 4, put);
        ps3rt_rsx_set_put(put);
    }
}
static void gcm_inject_clear_draw() {
    static const uint32_t kCols[] = { 0x101828u, 0x182030u, 0x203040u, 0x283848u };
    static int s_ci = 0;
    uint32_t col = kCols[s_ci++ & 3];
    gcm_fifo_emit(0x1D4, 0xF1);
    gcm_fifo_emit(0x1D5, col);
    gcm_fifo_emit(0x1D6, 0xFFFFFFFF);
    gcm_fifo_emit(0x1808, 1);
    static int s_log = 0;
    if (s_log < 3) { ps3rt_log("[gcm] inject clear+draw col=0x%06X\n", col); ++s_log; }
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
            uint32_t id = (uint32_t)c->gpr[3];
            if (id < 8) g_gcm_current_fb = id;
            if (g_mem) ps3rt_rsx_flush(g_mem);
            ps3rt_rsx_flip(id);
            g_gcm_flip_status = 0;
            ++g_gcm_frame_id;
            ++g_gcm_vblank;
            g_gcm_last_flip_time += 16666; // ~60Hz usec tick
            return ok();
        }
    case 0xF80196C0: // SetWaitFlip
        if (g_mem) ps3rt_rsx_flush(g_mem);
        ps3rt_rsx_flip(g_gcm_current_fb);
        g_gcm_flip_status = 0;
        ++g_gcm_vblank;
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
        // HLE: always complete
        c->gpr[3] = 0;
        return 0;
    default:
        c->gpr[3] = 0;
        return 0;
    }
}

static int prx_dispatch(PPUContext* c, uint32_t maybe_nid, int kind) {
    if (!c) return -1;
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
            // S: sysmodule load/unload/isLoaded
            if (e->nid == 0x3A495ADE || e->nid == 0xCFDD8E87 || e->nid == 0x3A52CF82) {
                prx_mod_load(e->name ? e->name : "sysmodule");
            }
            c->gpr[3] = 0; return 0;
        }
        if (kind == 8) {
            pad_poll_host();
            c->gpr[3] = 0; return 0;
        }
        // S: track first hit per module name as "loaded"
        if (e->module) prx_mod_load(e->module);
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
    if (kind == 8) { pad_poll_host(); c->gpr[3] = 0; return 0; }
    c->gpr[3] = 0; return 0;
}

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
    uint32_t maybe_nid = (uint32_t)c->gpr[11];
    if (maybe_nid == 0) maybe_nid = (uint32_t)(pc & 0xFFFFFFFFu);
    const NidEntry* e = nid_lookup(maybe_nid);
    int kind = e ? e->kind : 0;
    if (e && e->module) kind = prx_module_kind(e->module);
    static int s = 0;
    if (s < 24) {
        ps3rt_log("[ps3] prx_import_stub pc=0x%llx nid=0x%08X kind=%d\n",
            (unsigned long long)pc, maybe_nid, kind);
        ++s;
    }
    return prx_dispatch(c, maybe_nid, kind);
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
        if ((s_n & 0x7F) == 0) {
            uint32_t put = g_gcm_control ? rd_be32_guest(g_gcm_control + 0) : 0;
            if (put == 0) gcm_inject_clear_draw();
            else {
                static int s_guest = 0;
                if (s_guest < 3) { ps3rt_log("[gcm] XX-I guest put=0x%x\n", put); ++s_guest; }
                if (g_mem) ps3rt_rsx_flush(g_mem);
            }
            ps3rt_rsx_flip(0);
            g_gcm_flip_status = 0;
        }
    }
    static int s_run = 0;
    if (!s_run && g_gcm_inited) { PS3RT_STAGE("RUNNING"); s_run = 1; }
    if ((g_time_ppu_slices & 0x3FF) == 0) {
        static int s_t = 0;
        if (s_t < 8) {
            ps3rt_log("[time] X ppu=%llu spu=%llu rsx_flips=%llu phase=%u\n",
                (unsigned long long)g_time_ppu_slices,
                (unsigned long long)g_time_spu_ticks,
                (unsigned long long)g_time_rsx_flips, g_time_phase);
            ++s_t;
        }
    }
}

PS3RT_API void ps3rt_on_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 20) { ps3rt_log("[ps3] unimplemented op=0x%08X pc=0x%llx\n", opcode, (unsigned long long)pc); ++n; }
    if (c) c->halted = true;
}
