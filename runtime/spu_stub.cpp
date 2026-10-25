// XXV-H depth: SPU ISA + MFC tag completion
// XXIII-J SPURS completion
// XXIII-F: SPU/MFC tag update, list-stall ack, atomic HLE
// XXII-C: SPU/MFC fidelity (tag/stall surface)
// C/D/E: SPU ISA expand, MFC/DMA fidelity, SPURS jobchain
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstring>
#include <cstdint>
#include <cmath>

// SPU pool + main memory live in ps3rt.cpp / exports
extern SPUContext g_spus[];
extern int g_spu_count;
extern "C" void ps3rt_spu_interpreter_start(int id, uint32_t entry);

static inline uint8_t* main_mem() { return ps3rt_memory(); }
static inline uint64_t main_mem_size() { return 512ull * 1024 * 1024; }

static inline uint32_t be32_ls(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
static inline void wr_be32_ls(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);  p[3] = (uint8_t)v;
}
static inline uint32_t lr128_word(const uint8_t r[16], int word /*0=prefer preferred slot*/) {
    // SPU preferred slot is word 0 in big-endian register layout used here
    const uint8_t* p = r + (word & 3) * 4;
    return be32_ls(p);
}
static inline void sr128_word(uint8_t r[16], int word, uint32_t v) {
    wr_be32_ls(r + (word & 3) * 4, v);
}
static inline void r128_splat_u32(uint8_t r[16], uint32_t v) {
    for (int i = 0; i < 4; ++i) wr_be32_ls(r + i * 4, v);
}
static inline void r128_clear(uint8_t r[16]) { std::memset(r, 0, 16); }
static inline void r128_copy(uint8_t d[16], const uint8_t s[16]) { std::memcpy(d, s, 16); }

static inline uint32_t ls_addr(uint32_t a) { return a & 0x3FFF0u; } // 16-byte align within 256K

static void ls_load_quad(SPUContext& s, uint32_t addr, uint8_t dst[16]) {
    addr &= 0x3FFFF;
    addr &= ~0xFu;
    std::memcpy(dst, s.ls + addr, 16);
}
static void ls_store_quad(SPUContext& s, uint32_t addr, const uint8_t src[16]) {
    addr &= 0x3FFFF;
    addr &= ~0xFu;
    std::memcpy(s.ls + addr, src, 16);
}

// ---- Channel numbers (subset) ----
enum {
    CH_SPU_RdEventStat = 0,
    CH_SPU_WrEventMask = 1,
    CH_SPU_WrEventAck  = 2,
    CH_SPU_RdSigNotify1 = 3,
    CH_SPU_RdSigNotify2 = 4,
    CH_SPU_WrDec = 7,
    CH_SPU_RdDec = 8,
    CH_SPU_RdEventMask = 11,
    CH_MFC_WrMSSyncReq = 9,
    CH_SPU_RdMachStat = 15,
    CH_MFC_LSA = 16,
    CH_MFC_EAH = 17,
    CH_MFC_EAL = 18,
    CH_MFC_Size = 19,
    CH_MFC_TagID = 20,
    CH_MFC_Cmd = 21,
    CH_MFC_WrTagMask = 22,
    CH_MFC_WrTagUpdate = 23,
    CH_MFC_RdTagStat = 24,
    CH_MFC_RdListStallStat = 25,
    CH_MFC_WrListStallAck = 26,
    CH_MFC_RdAtomicStat = 27,
    CH_SPU_WrOutMbox = 28,
    CH_SPU_RdInMbox = 29,
    CH_SPU_WrOutIntrMbox = 30
};

// XX-L: MFC command set + list DMA + barriers + tag completion
static void mfc_complete_tag(SPUContext& s) {
    s.mfc_tag_status |= (1u << (s.mfc_tag & 31));
}
static int g_mfc_queue_depth[8] = {};

static void mfc_xfer(SPUContext& s, uint32_t lsa, uint64_t ea, uint32_t size, bool is_get) {
    if (!main_mem() || size == 0) return;
    lsa &= 0x3FFFF;
    if (ea + size > main_mem_size()) {
        if (ea >= main_mem_size()) return;
        size = (uint32_t)(main_mem_size() - ea);
    }
    if ((uint64_t)lsa + size > sizeof(s.ls))
        size = (uint32_t)(sizeof(s.ls) - lsa);
    if (is_get) std::memcpy(s.ls + lsa, main_mem() + ea, size);
    else std::memcpy(main_mem() + ea, s.ls + lsa, size);
}

static void mfc_list_dma(SPUContext& s, bool is_get) {

    uint32_t dst_lsa = s.mfc_lsa & 0x3FFFF;
    uint64_t list_ea = s.mfc_ea;
    uint32_t list_size = s.mfc_size & 0x7FFF;
    uint32_t done = 0;
    uint32_t elems = list_size / 8;
    if (elems == 0) elems = 1;
    if (elems > 2048) elems = 2048;
    for (uint32_t i = 0; i < elems; ++i) {
        uint64_t le_off = list_ea + (uint64_t)i * 8;
        if (le_off + 8 > main_mem_size()) { s.mfc_list_stall = 1; break; }
        uint8_t* p = main_mem() + le_off;
        uint16_t sz = (uint16_t)((p[0] << 8) | p[1]);
        uint64_t ea = ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
                      ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
                      ((uint64_t)p[6] << 8) | (uint64_t)p[7];
        if (sz == 0) break;
        if (sz > 16 * 1024) sz = 16 * 1024;
        mfc_xfer(s, (dst_lsa + done) & ~0xFu, ea, sz, is_get);
        done += (sz + 15u) & ~15u;
        if (done >= 256 * 1024) break;
    }
    static int s_list = 0;
    if (s_list < 16) {
        ps3rt_log("[spu] D MFC LIST %s elems=%u xfer=%u tag=%u stall=%u\n",
                  is_get ? "GETL" : "PUTL", elems, done, s.mfc_tag, s.mfc_list_stall);
        ++s_list;
    }
    mfc_complete_tag(s);

}

// XXV-H: uses existing mfc_complete_tag for ordering
static void mfc_do_dma(SPUContext& s) {
    uint32_t lsa = s.mfc_lsa & 0x3FFFF;
    uint64_t ea = s.mfc_ea;
    uint32_t size = s.mfc_size & 0x7FFF;
    uint32_t cmd = s.mfc_cmd & 0xFF;

    // Barriers / sync — complete immediately
    if (cmd == 0x40 /*PUTB*/ || cmd == 0x41 /*PUTBS*/ || cmd == 0x42 /*PUTBF*/ ||
        cmd == 0x44 /*PUTL*/ || cmd == 0x45 || cmd == 0x46 ||
        cmd == 0x48 /*BARRIER*/ || cmd == 0x50 /*MFCSYNC*/ || cmd == 0x58 /*MFCEIEIO*/) {
        if (cmd == 0x44 || cmd == 0x45 || cmd == 0x46) {
            mfc_list_dma(s, false);
            return;
        }
        mfc_complete_tag(s);
        return;
    }
    // GET list family
    if (cmd == 0x30 || cmd == 0x31 || cmd == 0x32 || cmd == 0x38 || cmd == 0x39 || cmd == 0x3A) {
        mfc_list_dma(s, true);
        return;
    }

    bool is_get = (cmd == 0x20 || cmd == 0x21 || cmd == 0x22 || cmd == 0x28 || cmd == 0x29 || cmd == 0x2A);
    bool is_put = (cmd == 0x24 || cmd == 0x25 || cmd == 0x26 || cmd == 0x2C || cmd == 0x2D || cmd == 0x2E);
    // Atomic: GETLLAR / PUTLLC / PUTLLUC — pretend success
    if (cmd == 0x60 || cmd == 0x61 || cmd == 0x62 || cmd == 0x64) {
        if (cmd == 0x60) mfc_xfer(s, lsa, ea, size ? size : 4, true);
        else if (cmd == 0x61 || cmd == 0x62) mfc_xfer(s, lsa, ea, size ? size : 4, false);
        s.mfc_atomic_stat = 0; // XXIII-F: HLE success (no real reservation yet)
        mfc_complete_tag(s);
        return;
    }
    if (!is_get && !is_put) {
        mfc_complete_tag(s);
        return;
    }
    mfc_xfer(s, lsa, ea, size, is_get);
    mfc_complete_tag(s);
    static int s_dma = 0;
    if (s_dma < 20) {
        ps3rt_log("[spu] D MFC %s lsa=0x%x ea=0x%llx size=%u tag=%u\n",
                  is_get ? "GET" : "PUT", lsa, (unsigned long long)ea, size, s.mfc_tag);
        ++s_dma;
    }
}

static uint32_t ch_read(SPUContext& s, uint32_t ch) {
    switch (ch) {
    case CH_SPU_RdInMbox:
        if (s.mailbox_in_count <= 0) return 0;
        {
            uint32_t v = s.mailbox_in[0];
            for (int i = 1; i < s.mailbox_in_count; ++i) s.mailbox_in[i - 1] = s.mailbox_in[i];
            --s.mailbox_in_count;
            return v;
        }
    case CH_SPU_RdEventStat:
        return s.ch_event_stat;
    case CH_SPU_RdEventMask:
        return s.ch_event_mask;
    case CH_SPU_RdDec:
        return s.decrementer;
    case CH_SPU_RdSigNotify1:
        return s.signal1;
    case CH_SPU_RdSigNotify2:
        return s.signal2;
    case CH_MFC_RdTagStat: { // XXII-C: report completed tags
        uint32_t m = s.mfc_tag_mask ? s.mfc_tag_mask : 0xFFFFFFFFu;
        return s.mfc_tag_status & m;
    }
    case CH_SPU_RdMachStat:
        // isolation / run bit: report running when interpreter active
        return s.running ? 1u : 0u;
    case CH_MFC_RdAtomicStat:
        return s.mfc_atomic_stat;
    case CH_MFC_RdListStallStat:
        return s.mfc_list_stall;
    default:
        return 0;
    }
}

static void ch_write(SPUContext& s, uint32_t ch, uint32_t val) {
    switch (ch) {
    case CH_SPU_WrOutMbox:
    case CH_SPU_WrOutIntrMbox:
        if (s.mailbox_out_count < 4)
            s.mailbox_out[s.mailbox_out_count++] = val;
        break;
    case CH_SPU_WrEventMask:
        s.ch_event_mask = val;
        break;
    case CH_SPU_WrEventAck:
        s.ch_event_stat &= ~val;
        break;
    case CH_SPU_WrDec:
        s.decrementer = val;
        break;
    case CH_MFC_LSA:
        s.mfc_lsa = val;
        break;
    case CH_MFC_EAH:
        s.mfc_ea = (s.mfc_ea & 0xFFFFFFFFull) | ((uint64_t)val << 32);
        break;
    case CH_MFC_EAL:
        s.mfc_ea = (s.mfc_ea & 0xFFFFFFFF00000000ull) | val;
        break;
    case CH_MFC_Size:
        s.mfc_size = val;
        break;
    case CH_MFC_TagID:
        s.mfc_tag = val & 31;
        break;
    case CH_MFC_Cmd: {
        int sid = (int)(&s - g_spus);
        if (sid >= 0 && sid < 8) {
            if (g_mfc_queue_depth[sid] >= 16) break;
            ++g_mfc_queue_depth[sid];
        }
        s.mfc_cmd = val; mfc_do_dma(s);
        if (sid >= 0 && sid < 8 && g_mfc_queue_depth[sid] > 0) --g_mfc_queue_depth[sid];
        break;
    }
    case CH_MFC_WrTagMask:
        s.mfc_tag_mask = val;
        break;
    case CH_MFC_WrTagUpdate: { // XXIII-F: mark masked tags complete
        s.mfc_tag_status |= (s.mfc_tag_mask ? s.mfc_tag_mask : 0xFFFFFFFFu);
        uint32_t m = s.mfc_tag_mask ? s.mfc_tag_mask : 0xFFFFFFFFu;
        if (val == 0) s.mfc_tag_status |= m;
        else if (val == 1) { if ((s.mfc_tag_status & m) == 0) s.mfc_tag_status |= m; }
        else s.mfc_tag_status |= m;
        break;
    }
    case CH_MFC_WrListStallAck:
        s.mfc_list_stall = 0; // XXIII-F
        break;
    case CH_MFC_WrMSSyncReq:
        break;
    default:
        break;
    }
}

static uint32_t ch_count(SPUContext& s, uint32_t ch) {
    switch (ch) {
    case CH_SPU_RdInMbox:
        return (uint32_t)s.mailbox_in_count;
    case CH_SPU_WrOutMbox:
    case CH_SPU_WrOutIntrMbox:
        return (uint32_t)(4 - s.mailbox_out_count);
    case CH_MFC_RdTagStat: // XXII-C: report completed tags
        return 1;
    case CH_MFC_Cmd:
        return 16; // always accept
    default:
        return 1;
    }
}

// Execute one SPU instruction; returns false if stopped
static bool spu_step(SPUContext& s) {
    if (!s.running) return false;
    uint32_t pc = s.pc & 0x3FFFC;
    uint32_t op = be32_ls(s.ls + pc);
    s.pc = (pc + 4) & 0x3FFFC;
    s.inst_count++;

    // Decode fields
    const uint32_t op_rt  = (op >> 0) & 0x7F;   // varies by format
    // RR:  opcode 11b | rb 7 | ra 7 | rt 7
    // RRR: op 4 | rc 7 | rb 7 | ra 7 | rt 7
    // RI7: op 11 | i7 7 | ra 7 | rt 7
    // RI10: op 8 | i10 10 | ra 7 | rt 7
    // RI16: op 9 | i16 16 | rt 7
    // RI18: op 7 | i18 18 | rt 7
    // LBT / other

    auto reg = [&](int i) -> uint8_t* { return s.gpr[i & 127]; };

    // stop / stopd  (01000000001 / 00101000000 variants)
    // stop: OP=0x000, RT=stop_code in bits — actually:
    // stop  opcode 00000000000 rt=7bit stop code in lower? 
    // Encoding: stop = 0b00000000000 | stop_signal(14?); common form 0x00000000 + code
    if ((op & 0xFFE00000u) == 0x00000000u && ((op >> 21) & 0x3FF) == 0) {
        // stop: bits 0..13 stop code in some docs; simpler: any 0x000xxxxx stop-like
    }
    // Standard:
    // stop  = 0000 0000 000 | stop_and_signal_type (7) ? 
    // From IBM: stop: opcode 0b00000000000, bits 25-31 = RT used as stop code
    if ((op >> 21) == 0x0) {
        // Could be lnop/nop/stop — distinguish:
        // lnop = 0x00000001, nop = 0x00200000
        if (op == 0x00200000u || op == 0x00000001u) return true; // nop/lnop
        if ((op & 0xFFE00000u) == 0) {
            uint32_t code = op & 0x3FFF;
            s.stop_status = (int)code;
            s.last_stop_code = code;
            s.running = false;
            if (s.mailbox_out_count < 4)
                s.mailbox_out[s.mailbox_out_count++] = code ? code : 1;
            static int s_stop = 0;
            if (s_stop < 12) {
                ps3rt_log("[spu] XX-K stop code=0x%x pc=0x%x insts=%llu\n",
                          code, pc, (unsigned long long)s.inst_count);
                ++s_stop;
            }
            return false;
        }
    }

    // --- Memory ---
    // lqd rt, symbol(ra)  OP=0x1<<3 ... encoding: 0011 0100 | i14 | ra | rt  → 0x34
    // Actually SPU encodings (from manuals):
    // lqd:  0b00110100 i10 ra rt  → opcode 0x34 >> wait
    // Prefer pattern match on high bits:

    uint32_t opc11 = (op >> 21) & 0x7FF;
    uint32_t opc8  = (op >> 24) & 0xFF;
    uint32_t opc9  = (op >> 23) & 0x1FF;
    uint32_t opc7  = (op >> 25) & 0x7F;
    uint32_t opc4  = (op >> 28) & 0xF;

    int rt = (int)(op & 0x7F);
    int ra = (int)((op >> 7) & 0x7F);
    int rb = (int)((op >> 14) & 0x7F);
    int rc = (int)((op >> 21) & 0x7F);

    // lqd: 0011 0100 i10 ra rt  (opc8=0x34)
    if (opc8 == 0x34) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        uint32_t addr = (lr128_word(reg(ra), 0) + ((int32_t)i10 << 4)) & 0x3FFFF;
        ls_load_quad(s, addr, reg(rt));
        return true;
    }
    // stqd: 0010 0100
    if (opc8 == 0x24) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        uint32_t addr = (lr128_word(reg(ra), 0) + ((int32_t)i10 << 4)) & 0x3FFFF;
        ls_store_quad(s, addr, reg(rt));
        return true;
    }
    // lqx: 0011 1000 100  rb ra rt
    if (opc11 == 0x1C4) {
        uint32_t addr = (lr128_word(reg(ra), 0) + lr128_word(reg(rb), 0)) & 0x3FFFF;
        ls_load_quad(s, addr, reg(rt));
        return true;
    }
    // stqx: 0010 1000 100
    if (opc11 == 0x144) {
        uint32_t addr = (lr128_word(reg(ra), 0) + lr128_word(reg(rb), 0)) & 0x3FFFF;
        ls_store_quad(s, addr, reg(rt));
        return true;
    }
    // lqa: 0011 0001 0 i16 rt
    if (opc9 == 0x61) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        uint32_t addr = ((uint32_t)(i16 << 2)) & 0x3FFFF;
        ls_load_quad(s, addr, reg(rt));
        return true;
    }
    // stqa: 0010 0001 0
    if (opc9 == 0x41) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        uint32_t addr = ((uint32_t)(i16 << 2)) & 0x3FFFF;
        ls_store_quad(s, addr, reg(rt));
        return true;
    }
    // lqr: 0011 0011 0 i16 rt  (relative)
    if (opc9 == 0x67) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        uint32_t addr = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFF;
        ls_load_quad(s, addr, reg(rt));
        return true;
    }
    // stqr: 0010 0011 0
    if (opc9 == 0x47) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        uint32_t addr = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFF;
        ls_store_quad(s, addr, reg(rt));
        return true;
    }

    // --- Immediate loads ---
    // il: 0100 0001 0 i16 rt
    if (opc9 == 0x81) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        r128_splat_u32(reg(rt), (uint32_t)i16);
        return true;
    }
    // ilh: 0100 0010 1 i16 rt
    if (opc9 == 0x85) {
        uint32_t i16 = (op >> 7) & 0xFFFF;
        r128_splat_u32(reg(rt), i16);
        return true;
    }
    // ilhu: 0100 0010 0 i16 rt
    if (opc9 == 0x84) {
        uint32_t i16 = (op >> 7) & 0xFFFF;
        r128_splat_u32(reg(rt), i16 << 16);
        return true;
    }
    // iohl: 0110 0001 0 i16 rt
    if (opc9 == 0xC1) {
        uint32_t i16 = (op >> 7) & 0xFFFF;
        for (int w = 0; w < 4; ++w) {
            uint32_t v = lr128_word(reg(rt), w);
            sr128_word(reg(rt), w, (v & 0xFFFF0000u) | i16);
        }
        return true;
    }
    // fsmbi: 0011 0010 1 i16 rt
    if (opc9 == 0x65) {
        uint32_t i16 = (op >> 7) & 0xFFFF;
        for (int b = 0; b < 16; ++b)
            reg(rt)[b] = (i16 & (1u << (15 - b))) ? 0xFF : 0x00;
        return true;
    }
    // ori: 0010 0000 i10 ra rt  (opc8=0x20 with form)
    // ai: 0001 1100 i10 ra rt
    if (opc8 == 0x1C) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) + (uint32_t)(int32_t)i10);
        return true;
    }
    // ahi: 0001 1101
    if (opc8 == 0x1D) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w) {
            // halfword pairs - simplify as 32-bit add of sign-ext
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) + (uint32_t)(int32_t)i10);
        }
        return true;
    }
    // sfi: 0001 0110
    if (opc8 == 0x16) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, (uint32_t)(int32_t)i10 - lr128_word(reg(ra), w));
        return true;
    }
    // andi: 0001 0100
    if (opc8 == 0x14) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) & (uint32_t)(int32_t)i10);
        return true;
    }
    // ori (RR with imm form 0000 0100): use opc
    if (opc8 == 0x04) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) | (uint32_t)(int32_t)i10);
        return true;
    }
    // xori: 0000 0101
    if (opc8 == 0x05) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) ^ (uint32_t)(int32_t)i10);
        return true;
    }
    // ceqi: 0111 1100
    if (opc8 == 0x7C) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w) {
            uint32_t a = lr128_word(reg(ra), w);
            sr128_word(reg(rt), w, a == (uint32_t)(int32_t)i10 ? 0xFFFFFFFFu : 0);
        }
        return true;
    }
    // cgti: 0100 1100
    if (opc8 == 0x4C) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w) {
            int32_t a = (int32_t)lr128_word(reg(ra), w);
            sr128_word(reg(rt), w, a > (int32_t)i10 ? 0xFFFFFFFFu : 0);
        }
        return true;
    }
    // clgti: 0101 1100
    if (opc8 == 0x5C) {
        int16_t i10 = (int16_t)((op >> 14) & 0x3FF);
        if (i10 & 0x200) i10 |= (int16_t)0xFC00;
        for (int w = 0; w < 4; ++w) {
            uint32_t a = lr128_word(reg(ra), w);
            sr128_word(reg(rt), w, a > (uint32_t)(int32_t)i10 ? 0xFFFFFFFFu : 0);
        }
        return true;
    }

    // --- RR arithmetic / logic ---
    // a: 0001 1000 000 rb ra rt
    if (opc11 == 0x0C0) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) + lr128_word(reg(rb), w));
        return true;
    }
    // sf: 0000 1000 000
    if (opc11 == 0x040) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(rb), w) - lr128_word(reg(ra), w));
        return true;
    }
    // and: 0001 1000 001
    if (opc11 == 0x0C1) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) & lr128_word(reg(rb), w));
        return true;
    }
    // or: 0000 1000 001
    if (opc11 == 0x041) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) | lr128_word(reg(rb), w));
        return true;
    }
    // xor: 0100 1000 001
    if (opc11 == 0x241) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) ^ lr128_word(reg(rb), w));
        return true;
    }
    // nand: 0001 1000 010
    if (opc11 == 0x0C2) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, ~(lr128_word(reg(ra), w) & lr128_word(reg(rb), w)));
        return true;
    }
    // nor: 0000 1000 010
    if (opc11 == 0x042) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, ~(lr128_word(reg(ra), w) | lr128_word(reg(rb), w)));
        return true;
    }
    // eqv: 0100 1000 010
    if (opc11 == 0x242) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, ~(lr128_word(reg(ra), w) ^ lr128_word(reg(rb), w)));
        return true;
    }
    // andc: 0101 1000 001
    if (opc11 == 0x2C1) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) & ~lr128_word(reg(rb), w));
        return true;
    }
    // orc: 0101 1000 010
    if (opc11 == 0x2C2) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) | ~lr128_word(reg(rb), w));
        return true;
    }
    // ceq: 0111 1000 000
    if (opc11 == 0x3C0) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) == lr128_word(reg(rb), w) ? 0xFFFFFFFFu : 0);
        return true;
    }
    // cgt: 0100 1000 000
    if (opc11 == 0x240) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, (int32_t)lr128_word(reg(ra), w) > (int32_t)lr128_word(reg(rb), w) ? 0xFFFFFFFFu : 0);
        return true;
    }
    // clgt: 0101 1000 000
    if (opc11 == 0x2C0) {
        for (int w = 0; w < 4; ++w)
            sr128_word(reg(rt), w, lr128_word(reg(ra), w) > lr128_word(reg(rb), w) ? 0xFFFFFFFFu : 0);
        return true;
    }
    // shli: 0x0F8 form — shl with imm from rb preferred
    // shl: 0000 1011 011
    if (opc11 == 0x05B) {
        for (int w = 0; w < 4; ++w) {
            uint32_t sh = lr128_word(reg(rb), 0) & 63;
            uint32_t a = lr128_word(reg(ra), w);
            sr128_word(reg(rt), w, sh >= 32 ? 0 : (a << sh));
        }
        return true;
    }
    // rothm: 0000 1011 100
    if (opc11 == 0x05C) {
        for (int w = 0; w < 4; ++w) {
            uint32_t sh = lr128_word(reg(rb), 0) & 63;
            uint32_t a = lr128_word(reg(ra), w);
            sr128_word(reg(rt), w, sh >= 32 ? 0 : (a >> sh));
        }
        return true;
    }
    // rot: 0000 1011 000
    if (opc11 == 0x058) {
        for (int w = 0; w < 4; ++w) {
            uint32_t sh = lr128_word(reg(rb), 0) & 31;
            uint32_t a = lr128_word(reg(ra), w);
            sr128_word(reg(rt), w, (a << sh) | (a >> ((32 - sh) & 31)));
        }
        return true;
    }
    // mpy: 0110 1100 100
    if (opc11 == 0x364) {
        for (int w = 0; w < 4; ++w) {
            int16_t a = (int16_t)(lr128_word(reg(ra), w) & 0xFFFF);
            int16_t b = (int16_t)(lr128_word(reg(rb), w) & 0xFFFF);
            sr128_word(reg(rt), w, (uint32_t)(int32_t)(a * b));
        }
        return true;
    }
    // mpyu: 0111 1100 100
    if (opc11 == 0x3E4) {
        for (int w = 0; w < 4; ++w) {
            uint16_t a = (uint16_t)(lr128_word(reg(ra), w) & 0xFFFF);
            uint16_t b = (uint16_t)(lr128_word(reg(rb), w) & 0xFFFF);
            sr128_word(reg(rt), w, (uint32_t)a * (uint32_t)b);
        }
        return true;
    }
    // selb: RRR 0100 rc rb ra rt
    if (opc4 == 0x4) {
        // selb: for each byte, select from rb or ra based on rc mask
        int rtc = rt, rac = ra, rbc = rb, rcc = rc;
        for (int b = 0; b < 16; ++b) {
            uint8_t mask = s.gpr[rcc][b];
            s.gpr[rtc][b] = (mask & 0x80) ? s.gpr[rbc][b] : s.gpr[rac][b];
        }
        return true;
    }
    // shufb: RRR 0011
    if (opc4 == 0x3) {
        int rtc = rt, rac = ra, rbc = rb, rcc = rc;
        uint8_t tmp[16];
        for (int b = 0; b < 16; ++b) {
            uint8_t pat = s.gpr[rcc][b];
            uint8_t sel = pat & 0x1F;
            if (sel < 16) tmp[b] = s.gpr[rac][sel];
            else if (sel < 32) tmp[b] = s.gpr[rbc][sel - 16];
            else if ((pat & 0xC0) == 0x80) tmp[b] = 0x00;
            else if ((pat & 0xC0) == 0xC0) tmp[b] = 0xFF;
            else tmp[b] = 0x00;
        }
        std::memcpy(s.gpr[rtc], tmp, 16);
        return true;
    }

    // --- Branches ---
    // br: 0011 0010 0 i16
    if (opc9 == 0x64) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        s.pc = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFC;
        return true;
    }
    // bra: 0011 0000 0 i16
    if (opc9 == 0x60) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        s.pc = ((uint32_t)(i16 << 2)) & 0x3FFFC;
        return true;
    }
    // brz: 0010 0000 0 i16 rt
    if (opc9 == 0x40) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        if (lr128_word(reg(rt), 0) == 0)
            s.pc = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFC;
        return true;
    }
    // brnz: 0010 0000 1
    if (opc9 == 0x42) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        if (lr128_word(reg(rt), 0) != 0)
            s.pc = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFC;
        return true;
    }
    // brhz: 0100 0000 0
    if (opc9 == 0x80) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        if ((lr128_word(reg(rt), 0) & 0xFFFF) == 0)
            s.pc = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFC;
        return true;
    }
    // brhnz: 0100 0000 1
    if (opc9 == 0x82) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        if ((lr128_word(reg(rt), 0) & 0xFFFF) != 0)
            s.pc = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFC;
        return true;
    }
    // brsl: 0011 0011 1 i16 rt  (branch relative set link)
    if (opc9 == 0x66) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        r128_splat_u32(reg(rt), (pc + 4) & 0x3FFFF);
        s.pc = (pc + ((uint32_t)(i16 << 2))) & 0x3FFFC;
        return true;
    }
    // brasl: 0011 0001 1
    if (opc9 == 0x62) {
        int32_t i16 = (int32_t)(int16_t)((op >> 7) & 0xFFFF);
        r128_splat_u32(reg(rt), (pc + 4) & 0x3FFFF);
        s.pc = ((uint32_t)(i16 << 2)) & 0x3FFFC;
        return true;
    }
    // bi: 0001 1010 100  ra  (branch indirect)
    if (opc11 == 0x1A8) {
        s.pc = lr128_word(reg(ra), 0) & 0x3FFFC;
        return true;
    }
    // bisl: 0001 1010 101 ra rt
    if (opc11 == 0x1A9) {
        r128_splat_u32(reg(rt), (pc + 4) & 0x3FFFF);
        s.pc = lr128_word(reg(ra), 0) & 0x3FFFC;
        return true;
    }
    // biz: 0001 0010 100 ra rt
    if (opc11 == 0x128) {
        if (lr128_word(reg(rt), 0) == 0)
            s.pc = lr128_word(reg(ra), 0) & 0x3FFFC;
        return true;
    }
    // binz: 0001 0010 101
    if (opc11 == 0x129) {
        if (lr128_word(reg(rt), 0) != 0)
            s.pc = lr128_word(reg(ra), 0) & 0x3FFFC;
        return true;
    }

    // XXII-I: SPU channel path (rdch/wrch) — keep above interpreter ops
    // --- Channels ---
    // wrch: 0010 0001 1 ra rt  (rt=channel, ra=data)  opc9?
    // wrch opcode 0x21A?  Documentation: wrch = 0010 0001 1 | ca | rt  where ca is channel
    // Encoding: 0100001101 | ra | rt  with channel in rt? 
    // Common: rdch rt, channel  / wrch channel, ra
    // opc11 for wrch = 0x10D (0100001101), channel = rt, value from ra? 
    if (opc11 == 0x10D) {
        ch_write(s, (uint32_t)rt, lr128_word(reg(ra), 0));
        return true;
    }
    // rdch: 0001 0001 1  channel in ra?  opc11=0x0D
    if (opc11 == 0x01D || opc11 == 0x00D) {
        uint32_t v = ch_read(s, (uint32_t)ra);
        r128_splat_u32(reg(rt), v);
        return true;
    }
    // rchcnt: 0001 0001 0
    if (opc11 == 0x00C || opc11 == 0x01C) {
        r128_splat_u32(reg(rt), ch_count(s, (uint32_t)ra));
        return true;
    }

    // sync / dsync / msync
    if (opc11 == 0x002 || opc11 == 0x003 || opc11 == 0x004) return true;

    // heq / hgt / hlgt — halt if condition (treat as nop if false, stop if true for safety)
    if (opc11 == 0x7F0 || opc11 == 0x4C0 || opc11 == 0x5C0) {
        // ignore halt-if for HLE
        return true;
    }

    
    // ---- C: expanded ISA ----
    if (opc11 == 0x241) { for (int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)|lr128_word(reg(rb),w)); return true; }
    if (opc11 == 0x1CE) { uint32_t v=0; for(int w=0;w<4;++w) v|=lr128_word(reg(ra),w); r128_clear(reg(rt)); sr128_word(reg(rt),0,v); return true; }
    if (opc11 == 0x0C3) { for(int w=0;w<4;++w) sr128_word(reg(rt),w,~(lr128_word(reg(ra),w)&lr128_word(reg(rb),w))); return true; }
    if (opc11 == 0x244) { for(int w=0;w<4;++w) sr128_word(reg(rt),w,~(lr128_word(reg(ra),w)^lr128_word(reg(rb),w))); return true; }
    if (opc11 == 0x0C2) { for(int w=0;w<4;++w){ uint64_t sum=(uint64_t)lr128_word(reg(ra),w)+lr128_word(reg(rb),w); sr128_word(reg(rt),w,(sum>>32)?1u:0u);} return true; }
    if (opc11 == 0x240) { for(int w=0;w<4;++w){ int32_t a=(int32_t)lr128_word(reg(ra),w),b=(int32_t)lr128_word(reg(rb),w); sr128_word(reg(rt),w,a>b?0xFFFFFFFFu:0);} return true; }
    if (opc11 == 0x3C0) { for(int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)==lr128_word(reg(rb),w)?0xFFFFFFFFu:0); return true; }
    if (opc11 == 0x1C7) { int sh=(int)(lr128_word(reg(rb),0)&15); uint8_t tmp[16]; r128_copy(tmp,reg(ra)); for(int i=0;i<16;++i) reg(rt)[i]=tmp[(i+sh)&15]; return true; }
    if (opc11 == 0x2C4 || opc11 == 0x2C5 || opc11 == 0x2C6) {
        for (int w=0;w<4;++w){ uint32_t ua=lr128_word(reg(ra),w),ub=lr128_word(reg(rb),w); float a,b; std::memcpy(&a,&ua,4); std::memcpy(&b,&ub,4);
            float r=(opc11==0x2C4)?a+b:((opc11==0x2C5)?a-b:a*b); uint32_t ur; std::memcpy(&ur,&r,4); sr128_word(reg(rt),w,ur);} return true; }
    if (opc11 == 0x1B6) { for(int w=0;w<4;++w){ uint32_t ua=lr128_word(reg(ra),w); float a; std::memcpy(&a,&ua,4); float r=(a!=0.f)?1.f/a:0.f; uint32_t ur; std::memcpy(&ur,&r,4); sr128_word(reg(rt),w,ur);} return true; }
    // XXV-H depth: more SPU arithmetic / logical
    if (opc11 == 0x0C0) { for (int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)+lr128_word(reg(rb),w)); return true; }
    if (opc11 == 0x0C8) { for (int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(rb),w)-lr128_word(reg(ra),w)); return true; }
    if (opc11 == 0x0C4) { for (int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)&lr128_word(reg(rb),w)); return true; }
    if (opc11 == 0x0C9) { for (int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)^lr128_word(reg(rb),w)); return true; }
    if (opc11 == 0x0C1) { for (int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)|lr128_word(reg(rb),w)); return true; }
    if (opc11 == 0x0CC) { int sh=(int)(lr128_word(reg(rb),0)&31); for(int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)<<sh); return true; }
    if (opc11 == 0x0CD) { int sh=(int)(lr128_word(reg(rb),0)&31); for(int w=0;w<4;++w) sr128_word(reg(rt),w,lr128_word(reg(ra),w)>>sh); return true; }
    if (opc11 == 0x3C4) { for(int w=0;w<4;++w){ int32_t a=(int32_t)lr128_word(reg(ra),w),b=(int32_t)lr128_word(reg(rb),w); sr128_word(reg(rt),w,a>b?0xFFFFFFFFu:0);} return true; }
    if (opc11 == 0x3C5) { for(int w=0;w<4;++w){ int32_t a=(int32_t)lr128_word(reg(ra),w),b=(int32_t)lr128_word(reg(rb),w); sr128_word(reg(rt),w,a<b?0xFFFFFFFFu:0);} return true; }
// Unknown — count and continue (skip)
    static int s_unk = 0;
    if (s_unk < 24) {
        ps3rt_log("[spu] C unhandled op=0x%08X pc=0x%x\n", op, pc);
        ++s_unk;
    }
    return true;
}

// Run up to max_steps instructions on one SPU
static void spu_run_slice(SPUContext& s, int max_steps) {
    if (!s.running) return;
    for (int i = 0; i < max_steps && s.running; ++i) {
        if (!spu_step(s)) break;
        if (s.decrementer) --s.decrementer;
    }
}

// XX-M: simple SPURS-like job slots
struct SpursJob {
    bool used; int spu_id; uint32_t entry; uint32_t prio; uint32_t workload_id; bool done;
    bool ready; uint32_t chain_next; uint32_t event_mask; uint32_t cause;
};
static SpursJob g_jobs[32];
static int g_spurs_inited = 0;
static uint32_t g_spurs_workload_seq = 1;

static int spurs_find_free_spu() {
    for (int i = 0; i < g_spu_count; ++i) {
        if (!g_spus[i].running && g_spus[i].spurs_idle)
            return i;
    }
    for (int i = 0; i < g_spu_count; ++i) {
        if (!g_spus[i].running) return i;
    }
    return -1;
}

static void spurs_schedule() {
    // Assign pending jobs to idle SPUs (higher prio first)
    for (int pass = 0; pass < 32; ++pass) {
        int best = -1;
        uint32_t best_prio = 0;
        for (int j = 0; j < 32; ++j) {
            if (!g_jobs[j].used || g_jobs[j].done || !g_jobs[j].ready || g_jobs[j].spu_id >= 0) continue;
            if (best < 0 || g_jobs[j].prio >= best_prio) {
                best = j; best_prio = g_jobs[j].prio;
            }
        }
        if (best < 0) break;
        int sid = spurs_find_free_spu();
        if (sid < 0) break;
        g_jobs[best].spu_id = sid;
        g_spus[sid].spurs_group = 0;
        g_spus[sid].spurs_job_prio = g_jobs[best].prio;
        g_spus[sid].spurs_workload_id = g_jobs[best].workload_id;
        g_spus[sid].spurs_idle = false;
        ps3rt_spu_interpreter_start(sid, g_jobs[best].entry);
        // XXIII-J: run a bounded slice immediately so jobs can complete
        if (sid >= 0 && sid < g_spu_count) {
            extern void spu_run_slice(int id, int max_steps); // may not exist
            // inline slice
            auto& s = g_spus[sid];
            for (int step = 0; step < 256 && s.running; ++step) {
                if (!spu_step(s)) break;
                if (s.decrementer) --s.decrementer;
            }
            if (!s.running) {
                g_jobs[best].done = true;
                g_jobs[best].cause = (uint32_t)s.last_stop_code;
                s.spurs_idle = true;
                if (g_jobs[best].chain_next != 0xFFFFFFFFu) {
                    uint32_t n = g_jobs[best].chain_next;
                    if (n < 32 && g_jobs[n].used) g_jobs[n].ready = true;
                }
            }
        }
        static int s_sched = 0;
        if (s_sched < 16) {
            ps3rt_log("[spurs] XXIII-J schedule job=%d -> spu=%d entry=0x%x prio=%u done=%d\n",
                      best, sid, g_jobs[best].entry, g_jobs[best].prio, g_jobs[best].done ? 1 : 0);
            ++s_sched;
        }
    }
    for (int j = 0; j < 32; ++j) {
        if (!g_jobs[j].used || g_jobs[j].done || g_jobs[j].spu_id < 0) continue;
        int sid = g_jobs[j].spu_id;
        if (sid < g_spu_count && !g_spus[sid].running) {
            g_jobs[j].done = true;
            g_jobs[j].cause = (uint32_t)g_spus[sid].last_stop_code;
            g_spus[sid].spurs_idle = true;
            g_spus[sid].spurs_group = -1;
            if (g_jobs[j].chain_next != 0xFFFFFFFFu) {
                uint32_t n = g_jobs[j].chain_next;
                if (n < 32 && g_jobs[n].used) g_jobs[n].ready = true;
            }
        }
    }
}

extern "C" PS3RT_API int ps3rt_spurs_submit_job(uint32_t entry, uint32_t prio, uint32_t workload_id) {
    for (int j = 0; j < 32; ++j) {
        if (g_jobs[j].used && !g_jobs[j].done) continue;
        g_jobs[j].used = true; g_jobs[j].spu_id = -1; g_jobs[j].entry = entry;
        g_jobs[j].prio = prio; g_jobs[j].workload_id = workload_id ? workload_id : g_spurs_workload_seq++;
        g_jobs[j].done = false; g_jobs[j].ready = true; g_jobs[j].chain_next = 0xFFFFFFFFu;
        g_jobs[j].event_mask = 0; g_jobs[j].cause = 0;
        spurs_schedule(); return j;
    }
    return -1;
}

extern "C" PS3RT_API int ps3rt_spurs_job_done(int job_id) {
    if (job_id < 0 || job_id >= 32 || !g_jobs[job_id].used) return 1;
    return g_jobs[job_id].done ? 1 : 0;
}

extern "C" PS3RT_API void ps3rt_spu_tick_all(void) {
    // XX-M: schedule SPURS jobs then time-slice running SPUs (prio bias)
    spurs_schedule();
    // Higher priority SPUs get larger slices
    for (int i = 0; i < g_spu_count; ++i) {
        if (!g_spus[i].running) continue;
        int steps = 256 + (int)(g_spus[i].spurs_job_prio & 0xFF);
        if (steps > 1024) steps = 1024;
        spu_run_slice(g_spus[i], steps);
    }
}


extern "C" PS3RT_API int ps3rt_spurs_link_chain(int job_id, int next_job_id) {
    if (job_id < 0 || job_id >= 32 || !g_jobs[job_id].used) return -1;
    if (next_job_id < 0 || next_job_id >= 32) { g_jobs[job_id].chain_next = 0xFFFFFFFFu; return 0; }
    g_jobs[job_id].chain_next = (uint32_t)next_job_id; g_jobs[next_job_id].ready = false; return 0;
}
extern "C" PS3RT_API int ps3rt_spurs_set_ready(int job_id, int ready) {
    if (job_id < 0 || job_id >= 32 || !g_jobs[job_id].used) return -1;
    g_jobs[job_id].ready = ready != 0; if (ready) spurs_schedule(); return 0;
}
// Called from ps3rt when run is requested — start interpreter
extern "C" void ps3rt_spu_interpreter_start(int id, uint32_t entry) {
    if (id < 0 || id >= g_spu_count) return;
    SPUContext& s = g_spus[id];
    s.pc = entry & 0x3FFFC;
    s.running = true;
    s.stop_status = 0;
    s.inst_count = 0;
    // Initial slice so short jobs complete before first PPU wait
    spu_run_slice(s, 1024);
    if (!s.running) {
        // already stopped and posted mbox in stop handler
    } else {
        // still running — cooperative; completion may come on later ticks
        static int s_run = 0;
        if (s_run < 8) {
            ps3rt_log("[spu] XX-K run id=%d entry=0x%x (interpreter, still running)\n", id, entry);
            ++s_run;
        }
    }
}
