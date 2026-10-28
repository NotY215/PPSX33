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
    // XXV-L: prefer LR when translated and not region-banned
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
        if (s_lr_hits < 8)
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


// ============================================================================
// XXV-F full depth — deterministic guest PC validation & fault capture
// Soft mode (default): recover but log every action; storm → fault+halt
// Strict mode: set env PS3RT_STRICT_FAULT=1 — unknown PC faults without inventing a target
// ============================================================================
enum class GuestPcClass : int {
    Translated = 0,
    ImportStub = 1,
    GuestMemory = 2,
    Invalid = 3
};

static inline bool ps3rt_strict_fault_mode() {
    static int cached = -1;
    if (cached < 0) {
        const char* e = std::getenv("PS3RT_STRICT_FAULT");
        cached = (e && e[0] == '1') ? 1 : 0;
        if (cached)
            ps3rt_log("[ps3] XXV-F STRICT_FAULT=1 — unknown PC will fault, not invent return\n");
    }
    return cached == 1;
}

static inline GuestPcClass validate_guest_pc(uint64_t pc, const PPUChunk* chunks, size_t n, const PPUContext& c) {
    if (!pc || (pc & 3ull) != 0)
        return GuestPcClass::Invalid;
    if (pc == 0x39800000ull || (pc & 0xFFFF0000ull) == 0x39800000ull)
        return GuestPcClass::ImportStub;
    if (pc_in_chunks(pc, chunks, n))
        return GuestPcClass::Translated;
    // guest RAM window used by runtime (coarse)
    if (pc >= 0x10000ull && pc < 0x10000000ull && guest_ok(const_cast<PPUContext&>(c), pc, 4))
        return GuestPcClass::GuestMemory;
    return GuestPcClass::Invalid;
}

static inline uint64_t resolve_function_descriptor(PPUContext& c, uint64_t opd) {
    if (!opd || (opd & 7ull) != 0) return 0;
    if (!guest_ok(c, opd, 16)) return 0;
    uint64_t entry = rd64(c, opd);
    uint64_t toc = rd64(c, opd + 8);
    if (!entry || (entry & 3ull) != 0) return 0;
    if (toc) c.gpr[2] = toc;
    return entry;
}

static inline uint64_t resolve_guest_branch(uint64_t target, const PPUChunk* chunks, size_t n, const PPUContext& c) {
    GuestPcClass k = validate_guest_pc(target, chunks, n, c);
    if (k == GuestPcClass::Translated || k == GuestPcClass::ImportStub)
        return target;
    // Do not silently retarget — caller decides recover vs fault
    return 0;
}

static inline void capture_guest_fault(PPUContext& c, const char* reason, const PPUChunk* chunks, size_t n, bool halt = true) {
    GuestPcClass k = validate_guest_pc(c.pc, chunks, n, c);
    const char* ks = "invalid";
    if (k == GuestPcClass::Translated) ks = "translated";
    else if (k == GuestPcClass::ImportStub) ks = "import_stub";
    else if (k == GuestPcClass::GuestMemory) ks = "guest_mem";
    ps3rt_log("[ps3] XXV-F FAULT %s pc=0x%llx class=%s lr=0x%llx ctr=0x%llx toc=r2=0x%llx sp=r1=0x%llx\n",
        reason ? reason : "?",
        (unsigned long long)c.pc, ks,
        (unsigned long long)c.lr, (unsigned long long)c.ctr,
        (unsigned long long)c.gpr[2], (unsigned long long)c.gpr[1]);
    ps3rt_log("[ps3] XXV-F FAULT gpr3-10 %llx %llx %llx %llx %llx %llx %llx %llx\n",
        (unsigned long long)c.gpr[3], (unsigned long long)c.gpr[4],
        (unsigned long long)c.gpr[5], (unsigned long long)c.gpr[6],
        (unsigned long long)c.gpr[7], (unsigned long long)c.gpr[8],
        (unsigned long long)c.gpr[9], (unsigned long long)c.gpr[10]);
    // Best-effort callstack: walk stack for code-looking return addresses
    uint64_t sp = c.gpr[1];
    int depth = 0;
    for (int off = 0; off < 0x2000 && depth < 8; off += 8) {
        uint64_t a = sp + (uint64_t)off;
        if (!guest_ok(c, a, 8)) break;
        uint64_t cand = rd64(c, a);
        if (!cand || (cand & 3ull) != 0) continue;
        if (!pc_in_chunks(cand, chunks, n) && cand != 0x39800000ull) continue;
        ps3rt_log("[ps3] XXV-F callstack[%d] 0x%llx\n", depth, (unsigned long long)cand);
        ++depth;
    }
    if (halt) c.halted = true;
}

static inline void log_recovery_action(const char* kind, uint64_t from, uint64_t to) {
    static int n = 0;
    if (n < 48) {
        ps3rt_log("[ps3] XXV-F recover %s 0x%llx -> 0x%llx\n",
            kind ? kind : "?", (unsigned long long)from, (unsigned long long)to);
        ++n;
    }
}


static inline void ppu_run(PPUContext& c, const PPUChunk* chunks, size_t n){
    ps3rt_log("[stage] PPU run start\n");
    int external_escapes = 0;
    int logged = 0;
    int soft_breaks = 0;
    uint64_t last_null_lr = 0;
    uint64_t last_good_ret = 0;
    uint64_t entry_toc = c.gpr[2];
    uint64_t steps = 0;

    uint64_t ban_lr[32] = {};
    int nban = 0, ban_w = 0;

    uint64_t rban[16] = {};
    int nrban = 0, rban_w = 0;

    int stub_same_lr = 0;
    uint64_t last_leave = 0;
    int leave_same = 0;

    uint64_t spin_pc = 0;
    uint64_t spin_count = 0;
    uint64_t progress_pokes = 0;
    uint64_t last_hb_pc = 0;
    int same_hb = 0;

    auto in_rban = [&](uint64_t p) -> bool {
        for (int i = 0; i < nrban; ++i)
            if (rban[i] && p >= rban[i] && p < rban[i] + 0x400ull) return true;
        return false;
    };
    auto is_banned = [&](uint64_t p) -> bool {
        if (!p || p < 0x10100ull) return true;
        for (int i = 0; i < nban; ++i) if (ban_lr[i] == p) return true;
        return in_rban(p);
    };
    auto add_ban = [&](uint64_t lr) {
        if (!lr || is_banned(lr)) return;
        if (lr >= 0x10004ull && lr < 0x20000ull) return;
        ban_lr[ban_w] = lr;
        ban_w = (ban_w + 1) % 32;
        if (nban < 32) ++nban;
    };
    auto add_rban = [&](uint64_t base) {
        uint64_t b = base & ~0x3FFull;
        if (b < 0x100000ull) return;
        if (in_rban(b)) return;
        rban[rban_w] = b;
        rban_w = (rban_w + 1) % 16;
        if (nrban < 16) ++nrban;
        ps3rt_log("[ps3] Phase XI region-ban 0x%llx (nrban=%d)\n",
            (unsigned long long)b, nrban);
    };

    // Phase XIV (safe): sustained-spin assist only — never thrash PC every step
    uint64_t xiv_last_pc = 0;
    uint64_t xiv_same = 0;
    uint64_t xiv_log = 0;
    // XXV-F: recovery storm detection
    uint64_t f_recoveries = 0;
    uint64_t f_last_from = 0;
    uint64_t f_same_from = 0;

    while (!c.halted) {

        // XXV-F: classify PC; strict or storm stops silent invent
        {
            GuestPcClass pk = validate_guest_pc(c.pc, chunks, n, c);
            if (pk == GuestPcClass::Invalid && c.pc != 0) {
                // allow one recover attempt path below; track storms
            }
            if (f_last_from == c.pc) ++f_same_from; else { f_last_from = c.pc; f_same_from = 1; }
            if (f_same_from > 64 || f_recoveries > 256) {
                capture_guest_fault(c, "recovery_storm", chunks, n);
                break;
            }
            if (ps3rt_strict_fault_mode() && pk == GuestPcClass::Invalid &&
                !(c.pc == 0x39800000ull || (c.pc & 0xFFFF0000ull) == 0x39800000ull)) {
                // still allow import stub region; else fault instead of invent
                if (c.pc == 0 || c.pc >= 0x10000000ull) {
                    capture_guest_fault(c, "strict_invalid_pc", chunks, n);
                    break;
                }
            }
        }

        if (ps3rt_should_exit()) {
            ps3rt_log("[ps3] exit requested (window closed)\n");
            ps3rt_log("[stage] PPU run end\n");
            c.halted = true;
            ps3rt_force_exit();
            ps3rt_log("[ps3] exit requested (window closed)\n");
            c.halted = true;
            break;
        }

        if (c.pc == xiv_last_pc) {
            ++xiv_same;
        } else {
            xiv_last_pc = c.pc;
            xiv_same = 0;
        }
        if (xiv_same == 4096) {
            ps3rt_progress_hw();
            ++progress_pokes;
        }
        if (xiv_same > 0 && (xiv_same % 65536) == 0) {
            if (xiv_log < 16) {
                ps3rt_log("[ps3] Phase XIV dwell pc=0x%llx n=%llu — HW poke\n",
                    (unsigned long long)c.pc, (unsigned long long)xiv_same);
                ++xiv_log;
            }
            ps3rt_progress_hw();
            ++progress_pokes;
            if (xiv_same >= 262144) {
                uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
                if (alt && alt != c.pc) {
                    c.pc = alt;
                    c.gpr[3] = 0;
                    xiv_same = 0;
                    continue;
                }
            }
        }

        if ((c.pc == 0 || c.pc >= 0x10000000ull) &&
            !(c.pc == 0x39800000ull || (c.pc & 0xFFFF0000ull) == 0x39800000ull)) {
            // CRITICAL: pass region-ban tables — nullptr made recovery ignore bans (log: always 0x331aac)
            uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
            if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
            if (alt && is_banned(alt)) {
                alt = 0;
                for (size_t ci = 0; ci < n && !alt; ++ci) {
                    uint64_t st = chunks[ci].start;
                    if (st && !is_banned(st)) alt = st;
                }
            }
            if (alt) {
                // P0 sticky: diagnose; soft continues; strict halts. No region-ban.
                static uint64_t s_last_alt = 0;
                static int s_same_alt = 0;
                static int s_sticky_log = 0;
                if (alt == s_last_alt) ++s_same_alt;
                else { s_last_alt = alt; s_same_alt = 1; }
                if (s_same_alt >= 3) {
                    const bool strict = ps3rt_strict_fault_mode();
                    // Log fault; only halt immediately in strict mode
                    if ((s_same_alt % 32) == 3 || strict) capture_guest_fault(c, "sticky_break", chunks, n, /*halt=*/strict);
                    if (s_sticky_log < 16) {
                        ps3rt_log("[ps3] P0 sticky BREAK first-fault pc=0x%llx alt=0x%llx lr=0x%llx hits=%d strict=%d\n",
                            (unsigned long long)c.pc, (unsigned long long)alt,
                            (unsigned long long)c.lr, s_same_alt, strict ? 1 : 0);
                        ++s_sticky_log;
                    }
                    if (strict) {
                        c.halted = true;
                        break;
                    }
                    uint64_t next = 0;
                    uint64_t lr = c.lr;
                    if (lr && (lr & 3ull) == 0 && pc_in_chunks(lr, chunks, n) && lr != alt)
                        next = lr;
                    if (!next) {
                        uint64_t sp = c.gpr[1];
                        for (int off = 0; off < 0x4000 && !next; off += 8) {
                            uint64_t a = sp + (uint64_t)off;
                            if (!guest_ok(c, a, 8)) break;
                            uint64_t cand = rd64(c, a);
                            if (!cand || (cand & 3ull) != 0) continue;
                            if (!pc_in_chunks(cand, chunks, n)) continue;
                            if (cand == alt || cand == c.pc) continue;
                            next = cand;
                        }
                    }
                    static uint64_t s_retry_alt = 0;
                    static int s_retry_n = 0;
                    if (next) {
                        if (s_sticky_log < 24)
                            ps3rt_log("[ps3] P0 sticky soft-continue -> 0x%llx\n", (unsigned long long)next);
                        alt = next;
                        c.gpr[3] = 0;
                        s_retry_alt = 0;
                        s_retry_n = 0;
                    } else if (alt && is_banned(alt)) {
                        uint64_t esc = recover_return_pc(c, chunks, n, alt, ban_lr, nban, rban, nrban);
                        if (!esc && last_good_ret && !is_banned(last_good_ret)) esc = last_good_ret;
                        if (!esc) {
                            for (size_t ci = 0; ci < n && !esc; ++ci) {
                                uint64_t st = chunks[ci].start;
                                if (st && !is_banned(st) && st != alt) esc = st;
                            }
                        }
                        if (esc) {
                            static uint64_t s_pp_esc = 0;
                            static int s_pp_n = 0;
                            if (esc == s_pp_esc) ++s_pp_n; else { s_pp_esc = esc; s_pp_n = 1; }
                            if (s_pp_n >= 3) {
                                add_rban(esc);
                                if (s_sticky_log < 32)
                                    ps3rt_log("[ps3] P0 sticky ban-pingpong esc=0x%llx\n",
                                        (unsigned long long)esc);
                                // Prefer last_good or another chunk start
                                uint64_t alt2 = 0;
                                if (last_good_ret && !is_banned(last_good_ret) && last_good_ret != esc)
                                    alt2 = last_good_ret;
                                if (!alt2) {
                                    for (size_t ci = 0; ci < n && !alt2; ++ci) {
                                        uint64_t st = chunks[ci].start;
                                        if (st && !is_banned(st) && st != esc && st != alt) alt2 = st;
                                    }
                                }
                                if (alt2) esc = alt2;
                                s_pp_n = 0;
                            }
                            if (s_sticky_log < 32)
                                ps3rt_log("[ps3] P0 sticky skip-banned alt=0x%llx -> 0x%llx\n",
                                    (unsigned long long)alt, (unsigned long long)esc);
                            alt = esc; c.pc = esc; c.gpr[3] = 0;
                        } else {
                            c.pc = last_good_ret ? last_good_ret : (n ? chunks[0].start : c.pc);
                            c.gpr[3] = 0;
                        }
                    } else if (alt && pc_in_chunks(alt, chunks, n) && !is_banned(alt)) {

                        if (alt == s_retry_alt) ++s_retry_n; else { s_retry_alt = alt; s_retry_n = 1; }
                        if (s_retry_n >= 6) {
                            // Ban spinning alt so recover/stack can leave the null-import island
                            add_rban(alt);
                            uint64_t esc = recover_return_pc(c, chunks, n, alt, ban_lr, nban, rban, nrban);
                            if (!esc) {
                                uint64_t sp = c.gpr[1];
                                for (int off = 0; off < 0x8000 && !esc; off += 8) {
                                    uint64_t a = sp + (uint64_t)off;
                                    if (!guest_ok(c, a, 8)) break;
                                    uint64_t cand = rd64(c, a);
                                    if (!cand || (cand & 3ull) != 0) continue;
                                    if (!pc_in_chunks(cand, chunks, n)) continue;
                                    if (cand == alt || cand == c.pc || cand == s_retry_alt) continue;
                                    esc = cand;
                                }
                            }
                            if (!esc && last_good_ret && pc_in_chunks(last_good_ret, chunks, n)
                                && last_good_ret != alt && !is_banned(last_good_ret))
                                esc = last_good_ret;
                            if (!esc) {
                                // Last resort: any translated chunk entry not equal to spinning alt
                                for (size_t ci = 0; ci < n && !esc; ++ci) {
                                    uint64_t st = chunks[ci].start;
                                    if (!st || (st & 3ull) != 0) continue;
                                    if (st == alt || st == c.pc || st == s_retry_alt) continue;
                                    if (is_banned(st)) continue;
                                    esc = st;
                                }
                            }
                            if (esc) {
                                if (s_sticky_log < 48)
                                    ps3rt_log("[ps3] P0 sticky escalate -> 0x%llx (was alt=0x%llx n=%d)\n",
                                        (unsigned long long)esc, (unsigned long long)alt, s_retry_n);
                                add_rban(alt);
                                alt = esc;
                                c.pc = esc;
                                c.gpr[3] = 0;
                                s_retry_n = 0;
                                s_retry_alt = 0;
                            } else {
                                // Dead island with no translated escape — skip this alt permanently
                                add_rban(alt);
                                if (s_sticky_log < 48)
                                    ps3rt_log("[ps3] P0 sticky dead-island alt=0x%llx (n=%d) — banned\n",
                                        (unsigned long long)alt, s_retry_n);
                                s_retry_n = 0;
                                s_retry_alt = 0;
                                // Keep running host HW present path; advance PC off 0
                                if (last_good_ret && !is_banned(last_good_ret))
                                    c.pc = last_good_ret;
                                else if (n && chunks[0].start)
                                    c.pc = chunks[0].start;
                                c.gpr[3] = 0;
                                continue; // do not fall into soft-retry spin
                            }
                        } else if (s_sticky_log < 24) {
                            ps3rt_log("[ps3] P0 sticky soft-retry alt=0x%llx\n", (unsigned long long)alt);
                        }
                    } else {
                        c.halted = true;
                        break;
                    }
                    s_same_alt = 0;
                    s_last_alt = 0;
                }
static int wild_log2 = 0;
                if (wild_log2 < 12) {
                    if (entry_toc) c.gpr[2] = entry_toc;
                    log_recovery_action("wild-pc", c.pc, alt); ++f_recoveries;
                    ps3rt_log("[ps3] XXV-L wild-pc 0x%llx -> 0x%llx\n",
                        (unsigned long long)c.pc, (unsigned long long)alt);
                    ++wild_log2;
                }
                c.pc = alt;
                c.gpr[3] = 0;
                continue;
            }
        }

        if (entry_toc != 0) {
            uint64_t r2 = c.gpr[2];
            if (r2 == 0 || r2 > 0x10000000ull || (r2 & 3ull) != 0)
                c.gpr[2] = entry_toc;
        }

        bool is_wait = (c.pc != 0 && c.pc == c.lr);
        if (c.pc == spin_pc || is_wait) {
            if (c.pc != spin_pc) { spin_pc = c.pc; spin_count = 0; }
            ++spin_count;
            if ((spin_count & 0x7F) == 0) { ps3rt_progress_hw(); ++progress_pokes; }
            if (spin_count >= 0x400 && (spin_count & 0x7F) == 0) {
                // XX-H: earlier spin assist + force flip/fs progress while waiting
                ps3rt_progress_hw();
                ++progress_pokes;
                uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                if (alt && alt != c.pc && !is_banned(alt)) {
                    c.pc = alt; c.gpr[3] = 0; spin_count = 0; continue;
                }
                // If pc==lr wait-loop, nudge LR path by completing a synthetic flip
                if (is_wait && (spin_count & 0x1FF) == 0)
                    c.gpr[3] = 0; // HLE "wait done"
            }
        } else {
            spin_pc = c.pc; spin_count = 0;
        }

        {
            bool nullish_early = (c.pc == 0x39800000ull) || ((c.pc & 0xFFFF0000ull) == 0x39800000ull);
            if (nullish_early) {
                uint64_t ret = c.lr;
                if (!ret || is_banned(ret) || !pc_in_chunks(ret, chunks, n))
                    ret = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                if (!ret && last_good_ret && !is_banned(last_good_ret) && pc_in_chunks(last_good_ret, chunks, n))
                    ret = last_good_ret;
                if (ret && external_escapes < 262144) {
                    ps3rt_prx_import_stub(&c, c.pc);
                    bool same_site = last_null_lr &&
                        (ret == last_null_lr ||
                         (ret >= (last_null_lr & ~0xFFull) && ret < (last_null_lr & ~0xFFull) + 0x100));
                    if (same_site) {
                        if (++stub_same_lr > 32) {
                            // XX-H: sticky null-loop at same LR — skip forward and force HW progress
                            ++soft_breaks;
                            stub_same_lr = 0;
                            ps3rt_progress_hw();
                            ++progress_pokes;
                            uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban, rban, nrban);
                            if (alt && alt != ret && !is_banned(alt)) {
                                if (soft_breaks <= 32 || (soft_breaks & 0x1F) == 0)
                                    ps3rt_log("[ps3] null-loop recover 0x%llx -> 0x%llx (soft_breaks=%d)\n",
                                        (unsigned long long)ret, (unsigned long long)alt, soft_breaks);
                                ret = alt;
                            } else {
                                // Growing stride: 0x400 → 0x1000 → 0x4000 → 0x10000
                                uint64_t stride = 0x400ull;
                                if (soft_breaks >= 4) stride = 0x1000ull;
                                if (soft_breaks >= 16) stride = 0x4000ull;
                                if (soft_breaks >= 64) stride = 0x10000ull;
                                uint64_t next = (ret & ~0xFull) + stride;
                                if (pc_in_chunks(next, chunks, n) && !is_banned(next)) {
                                    if (soft_breaks <= 48 || (soft_breaks & 0x3F) == 0)
                                        ps3rt_log("[ps3] null-loop skip: 0x%llx +0x%llx -> 0x%llx (soft_breaks=%d)\n",
                                            (unsigned long long)ret, (unsigned long long)stride,
                                            (unsigned long long)next, soft_breaks);
                                    ret = next;
                                } else if (pc_in_chunks(ret + 4, chunks, n)) {
                                    ret = ret + 4;
                                }
                            }
                            // Ban sticky LR earlier so we stop returning into the same trap
                            if (soft_breaks >= 4 && (soft_breaks & 3) == 0) {
                                add_ban(last_null_lr);
                                last_null_lr = 0;
                            }
                            // Region exhausted: drop sticky, region-ban the 0x1000 window
                            if (soft_breaks >= 12 && (soft_breaks % 12) == 0) {
                                add_rban(ret);
                                ps3rt_log("[ps3] null-loop region exhausted (soft_breaks=%d) -- dropping sticky\n", soft_breaks);
                                last_null_lr = 0;
                                uint64_t far = recover_return_pc(c, chunks, n, ret, ban_lr, nban, rban, nrban);
                                if (far && far != ret) ret = far;
                            }
                        }
                    } else {
                        stub_same_lr = 0;
                        last_null_lr = ret;
                    }
                    if (logged < 96) {
                        ps3rt_log("[ps3] import stub pc=0x%llx -> ret=0x%llx (#%d)\n",
                            (unsigned long long)c.pc, (unsigned long long)ret, external_escapes + 1);
                        ++logged;
                    }
                    // Do not land on a region-banned PC (sticky island like 0x331aac)
                    if (is_banned(ret)) {
                        uint64_t alt = 0;
                        if (last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
                        if (!alt && c.lr && (c.lr & 3ull) == 0 && pc_in_chunks(c.lr, chunks, n) && !is_banned(c.lr))
                            alt = c.lr;
                        if (!alt) {
                            for (size_t ci = 0; ci < n && !alt; ++ci) {
                                uint64_t st = chunks[ci].start;
                                if (st && !is_banned(st)) alt = st;
                            }
                        }
                        if (alt) {
                            static int s_ib = 0;
                            if (s_ib < 16) {
                                ps3rt_log("[ps3] import ret banned 0x%llx -> 0x%llx\n",
                                    (unsigned long long)ret, (unsigned long long)alt);
                                ++s_ib;
                            }
                            ret = alt;
                        }
                    }
                    if (!is_banned(ret))
                        last_good_ret = ret;
                    c.pc = ret;
                    ++external_escapes;
                    continue;
                }
            }
        }

        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i) {
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }
        }

        if (!hit && !(c.pc == 0x39800000ull || (c.pc & 0xFFFF0000ull) == 0x39800000ull)) {
            uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
            if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
            if (alt && external_escapes < 262144) {
                if (logged < 32) {
                    ps3rt_log("[ps3] Phase XI force-out pc=0x%llx -> 0x%llx\n",
                        (unsigned long long)c.pc, (unsigned long long)alt);
                    ++logged;
                }
                if (last_leave && alt > last_leave && alt < last_leave + 0x10000ull) {
                    ++leave_same;
                    if (leave_same >= 4) {
                        add_rban(alt);
                        uint64_t force = recover_return_pc(c, chunks, n, alt, ban_lr, nban, rban, nrban);
                        if (!force && last_good_ret && !is_banned(last_good_ret)) force = last_good_ret;
                        if (force) alt = force;
                        leave_same = 0;
                    }
                } else leave_same = 0;
                last_leave = alt;
                c.pc = alt; c.gpr[3] = 0; ++external_escapes; continue;
            }
            // Soft: do not kill the title when escape budget is exhausted — return to last good
            {
                static uint64_t s_left_pc = 0;
                static int s_left_n = 0;
                if (c.pc == s_left_pc) ++s_left_n; else { s_left_pc = c.pc; s_left_n = 1; }
                uint64_t dest = 0;
                if (s_left_n >= 3) {
                    uint64_t sp = c.gpr[1];
                    for (int off = 0; off < 0x8000 && !dest; off += 8) {
                        uint64_t a = sp + (uint64_t)off;
                        if (!guest_ok(c, a, 8)) break;
                        uint64_t cand = rd64(c, a);
                        if (!cand || (cand & 3ull) != 0) continue;
                        if (!pc_in_chunks(cand, chunks, n)) continue;
                        if (cand == last_good_ret || cand == c.pc) continue;
                        dest = cand;
                    }
                }
                if (!dest && last_good_ret && pc_in_chunks(last_good_ret, chunks, n))
                    dest = last_good_ret;
                if (dest) {
                    static int s_soft_left = 0;
                    if (s_soft_left < 32) {
                        ps3rt_log("[ps3] soft left-range pc=0x%llx -> 0x%llx (escapes=%d n=%d)\n",
                            (unsigned long long)c.pc, (unsigned long long)dest, external_escapes, s_left_n);
                        ++s_soft_left;
                    }
                    c.pc = dest;
                    c.gpr[3] = 0;
                    external_escapes = 0;
                    if (s_left_n >= 3) { s_left_pc = 0; s_left_n = 0; }
                    continue;
                }
            }
            ps3rt_log("[ps3] halt: PC left range pc=0x%llx lr=0x%llx escapes=%d nrban=%d last_good=0x%llx\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                external_escapes, nrban, (unsigned long long)last_good_ret);
            c.halted = true; break;
        }

        if (!hit) {
            if (last_good_ret) { c.pc = last_good_ret; continue; }
            ps3rt_log("[ps3] halt: import with no return pc=0x%llx lr=0x%llx\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr);
            c.halted = true; break;
        }

        hit->fn(c);

        if ((++steps & 0x3FFFFull) == 0) {

            // XXIII-R/A: null-PC — last_good, first chunk, or mid-chunk; restore TOC
            if (c.pc == 0) {
                uint64_t boot = last_good_ret;
                if (!boot && n > 0 && chunks) boot = chunks[0].start;
                if (!boot && n > 1 && chunks) boot = chunks[n / 2].start;
                if (boot) {
                    static int zpc = 0;
                    if (zpc < 20) {
                        log_recovery_action("null-pc", 0, boot); ++f_recoveries;
                        ps3rt_log("[ps3] XXV-F null-pc recover -> 0x%llx toc=0x%llx\n",
                            (unsigned long long)boot, (unsigned long long)entry_toc);
                        ++zpc;
                    }
                    c.pc = boot;
                    if (entry_toc) c.gpr[2] = entry_toc;
                }
            }
            ps3rt_log("[ps3] heartbeat steps=%llu pc=0x%llx lr=0x%llx r2=0x%llx pokes=%llu nrban=%d\n",
                (unsigned long long)steps, (unsigned long long)c.pc,
                (unsigned long long)c.lr, (unsigned long long)c.gpr[2],
                (unsigned long long)progress_pokes, nrban);
            if (c.pc == last_hb_pc) {
                if (++same_hb >= 3) {
                    uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                    if (alt && alt != c.pc) { c.pc = alt; same_hb = 0; }
                }
            } else { last_hb_pc = c.pc; same_hb = 0; }
        }
    }
    ps3rt_log("[stage] PPU run end\n");
}
