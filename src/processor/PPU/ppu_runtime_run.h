// WORK: PPU run support (ppu_runtime_run.h).


// HLE status dump is fully header-local (no hle_status.h required for game builds).
static inline void ps3rt_log_hle_status_once(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    static const char* const k_names[] = {
        "sys_ppu_thread/mutex/cond", "sys_memory/mmapper", "cellFs", "cellPad",
        "cellAudio", "cellGcmSys", "cellSysutil/sysmodule", "cellSpurs", "RSX/FIFO"
    };
    static const char* const k_notes[] = {
        "subset", "subset", "path map + host I/O", "XInput/host keys",
        "host path", "control/FIFO kick", "common NIDs", "job assist",
        "assist draws proven; title stream pending"
    };
    for (int i = 0; i < 9; ++i)
        ps3rt_log("[hle] %s = partial (%s)\n", k_names[i], k_notes[i]);
}

// XXV-L: sticky wild-pc break + LR-first recovery
// XXV-F full depth: validate_guest_pc, capture_guest_fault, strict/storm
#include <cstdlib>
// XXIII-R: PPU control-flow TOC/OPD recovery
// XXII-H: PPU recovery/run surface — prefer soft continue over halt
// Phase XV: import stub before force-out; wild-PC excludes 0x39800000
// Phase XIV (safe): sustained dwell assist — no PC thrash storm
// XX-H: stronger null-loop skip, earlier spin assist, sticky region drop
static inline uint64_t recover_return_pc(PPUContext& c, const PPUChunk* chunks, size_t n,
                                         uint64_t exclude_pc = 0,
                                         const uint64_t* ban = nullptr, int nban = 0,
                                         const uint64_t* rban = nullptr, int nrban = 0) {
    // XXV-L / XXVI: prefer LR when translated and not region-banned.
    // Hardening: raise sticky-hit budget and always prefer a fresh LR that is
    // in-chunks over stack scan (import-island null loops often leave LR valid).
    auto banned_early = [&](uint64_t p) -> bool {
        if (!p || p == exclude_pc) return true;
        if (p < 0x10100ull) return true;
        for (int i = 0; i < nban; ++i) if (ban && ban[i] == p) return true;
        for (int i = 0; i < nrban; ++i) {
            uint64_t b = rban ? rban[i] : 0;
            if (b && p >= b && p < b + 0x400ull) return true;
        }
        return false;
    };
    if (c.lr && (c.lr & 3ull) == 0 && pc_in_chunks(c.lr, chunks, n) && !banned_early(c.lr)) {
        static uint64_t s_lr_sticky = 0;
        static int s_lr_hits = 0;
        if (c.lr == s_lr_sticky) ++s_lr_hits; else { s_lr_sticky = c.lr; s_lr_hits = 1; }
        // XXVI: allow more consecutive LR hits before falling through to stack
        // (was 8; sticky import recovery needs headroom under soft-continue)
        if (s_lr_hits < 32)
            return c.lr;
        // After budget, still return LR once more if it differs from exclude_pc
        if (c.lr != exclude_pc)
            return c.lr;
    }

    auto banned = [&](uint64_t p) -> bool {
        if (!p || p == exclude_pc) return true;
        if (p < 0x10100ull) return true;
        for (int i = 0; i < nban; ++i) if (ban[i] == p) return true;
        for (int i = 0; i < nrban; ++i) {
            uint64_t b = rban[i];
            if (b && p >= b && p < b + 0x400ull) return true;
        }
        return false;
    };
    if (c.lr && !banned(c.lr) && (c.lr & 3ull) == 0 && pc_in_chunks(c.lr, chunks, n))
        return c.lr;
    if (c.ctr && !banned(c.ctr) && (c.ctr & 3ull) == 0 && pc_in_chunks(c.ctr, chunks, n))
        return c.ctr;
    uint64_t sp = c.gpr[1];
    uint64_t best_hi = 0;
    for (int dir = 0; dir < 2; ++dir) {
        int start = (dir == 0) ? 0 : -0x1000;
        int end   = (dir == 0) ? 0x4000 : 0;
        for (int off = start; off <= end; off += 8) {
            uint64_t a = sp + (uint64_t)(int64_t)off;
            if (!guest_ok(c, a, 8)) continue;
            uint64_t cand = rd64(c, a);
            if (banned(cand) || (cand & 3ull) != 0) continue;
            if (!pc_in_chunks(cand, chunks, n)) continue;
            if (cand >= 0x300000ull && cand < 0x500000ull) return cand;
            if (cand >= 0x20000ull && !best_hi) best_hi = cand;
            if (!best_hi) best_hi = cand;
        }
    }
    if (best_hi) return best_hi;
    // XXIII-R: try function descriptor via TOC (r2): OPD = [entry,toc]
    uint64_t toc = c.gpr[2];
    if (toc && guest_ok(c, toc, 16)) {
        for (int k = 0; k < 8; ++k) {
            uint64_t opd = toc + (uint64_t)k * 8ull;
            if (!guest_ok(c, opd, 16)) break;
            uint64_t ent = rd64(c, opd);
            if (!banned(ent) && (ent & 3ull) == 0 && pc_in_chunks(ent, chunks, n))
                return ent;
        }
    }
    return 0;
}
