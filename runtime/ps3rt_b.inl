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
    case 1: c->halted = true; c->gpr[3] = 0; break;
    case 141: case 142: case 144: case 145: case 48: case 49: case 50:
    case 52: case 53: case 54: case 70: case 71: case 72: case 90: case 91:
    case 120: case 121: case 129: case 130: case 131: case 132: case 133: case 134:
    case 114: case 115: case 116: case 169: case 348: case 349: case 350: case 351:
    case 353: case 354: case 355: case 403: case 801: case 802: case 803:
    case 871: case 872: case 25602: c->gpr[3] = 0; break;
    case 128: {
        static uint32_t eid = 0x7000; ++eid;
        uint64_t out = r3 ? r3 : r4;
        if (out && out + 4 <= g_mem_size) wr_be32_guest(out, eid);
        c->gpr[3] = 0; break;
    }
    case 143: { static uint32_t fid = 0x8000; if (r3 && r3+4 <= g_mem_size) wr_be32_guest(r3, ++fid); c->gpr[3] = 0; break; }
    case 352: {
        uint64_t size = r3 ? r3 : 0x100000;
        if (size > 0x1000000) size = 0x1000000;
        uint64_t a = guest_alloc(size, 0x10000);
        if (r4 && r4 + 8 <= g_mem_size) wr_be64_guest(r4, a);
        c->gpr[3] = a ? 0 : (uint64_t)-1; break;
    }
    default: c->gpr[3] = 0; break;
    }
}

static int g_prx_log = 0;
static uint64_t g_gcm_fifo = 0;
static uint32_t g_gcm_fifo_size = 0x100000;
static int g_gcm_inited = 0;
static uint64_t g_gcm_control = 0;
static uint64_t g_gcm_labels = 0;
static uint32_t g_gcm_flip_status = 0;
static uint32_t g_gcm_frame_id = 0;
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
        ps3rt_log("[gcm] Phase G state ready fifo=0x%llx ctrl=0x%llx\n", (unsigned long long)g_gcm_fifo, (unsigned long long)g_gcm_control);
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
            gcm_ensure_state();
            if (e->nid == 0x55A14C05 && c->gpr[3] && c->gpr[3]+8 <= g_mem_size) {
                wr_be64_guest(c->gpr[3], g_gcm_control); c->gpr[3] = 0; return 0;
            }
            if (e->nid == 0x5F909B17 && c->gpr[3] && c->gpr[3]+8 <= g_mem_size) {
                wr_be64_guest(c->gpr[3], g_gcm_labels); c->gpr[3] = 0; return 0;
            }
            if (e->nid == 0xA547ADDE) { c->gpr[3] = g_gcm_flip_status; return 0; }
            if (e->nid == 0xB2E761D4) { g_gcm_flip_status = 0; c->gpr[3] = 0; return 0; }
            if (e->nid == 0xD81D0D2D || e->nid == 0x05E548B9) {
                gcm_inject_clear_draw();
                ps3rt_rsx_flip();
                g_gcm_flip_status = 0; ++g_gcm_frame_id; c->gpr[3] = 0; return 0;
            }
            if (e->nid == 0xF80196C0) { c->gpr[3] = 0; return 0; }
            c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x1C936E6D) { g_pad_inited = 1; pad_poll_host(); c->gpr[3] = 0; return 0; }
        if (e->nid == 0x0D5F2C14) { g_pad_inited = 0; c->gpr[3] = 0; return 0; }
        if (e->nid == 0x3AAAD464) {
            pad_poll_host();
            uint64_t out = c->gpr[4] ? c->gpr[4] : c->gpr[3];
            pad_write_data(out); c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0xA703A51D) {
            pad_poll_host();
            uint64_t out = c->gpr[3] ? c->gpr[3] : c->gpr[4];
            pad_write_info2(out); c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x3F797DFF) { c->gpr[3] = 0; return 0; }
        if (kind == 4) { return ps3rt_audio_dispatch(c, e->nid); }
        if (kind == 11) { return ps3rt_save_dispatch(c, e->nid); }
        if (kind == 8) {
            pad_poll_host();
            c->gpr[3] = 0; return 0;
        }
        c->gpr[3] = 0; return 0;
    }
    if (kind == 4) { return ps3rt_audio_dispatch(c, maybe_nid); }
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

PS3RT_API void ps3rt_progress_hw(void) {
    if (ps3rt_should_exit()) { ps3rt_force_exit(); return; }
    static int s_ppu_once = 0;
    if (!s_ppu_once) { PS3RT_STAGE("PPU progress start"); s_ppu_once = 1; }
    spu_tick_all();
    pad_poll_host();
    ps3rt_audio_tick();
    if (!g_gcm_inited) gcm_ensure_state();
    if (g_gcm_inited) {
        static uint32_t s_n = 0;
        if ((++s_n & 0x3F) == 0) {
            uint32_t put = g_gcm_control ? rd_be32_guest(g_gcm_control + 0) : 0;
            if (put == 0) gcm_inject_clear_draw();
            else {
                static int s_guest = 0;
                if (s_guest < 3) { ps3rt_log("[gcm] Phase XII guest put 0x0 -> 0x%x\n", put); ++s_guest; }
            }
            ps3rt_rsx_flip();
        }
    }
    static int s_run = 0;
    if (!s_run && g_gcm_inited) { PS3RT_STAGE("RUNNING"); s_run = 1; }
}

PS3RT_API void ps3rt_on_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 20) { ps3rt_log("[ps3] unimplemented op=0x%08X pc=0x%llx\n", opcode, (unsigned long long)pc); ++n; }
    if (c) c->halted = true;
}
