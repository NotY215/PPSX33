// Phase XI+XIII: never walk-ban the image; sticky leave prefers high last_good / stack; wild-PC recovery
static inline uint64_t recover_return_pc(PPUContext& c, const PPUChunk* chunks, size_t n,
                                         uint64_t exclude_pc = 0,
                                         const uint64_t* ban = nullptr, int nban = 0,
                                         const uint64_t* rban = nullptr, int nrban = 0) {
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
    return 0;
}

static inline void ppu_run(PPUContext& c, const PPUChunk* chunks, size_t n){
    int external_escapes = 0;
    int logged = 0;
    int soft_breaks = 0;
    uint64_t last_null_lr = 0;
    uint64_t last_good_ret = 0;
    uint64_t failed_escape = 0;
    int escape_fails = 0;
    uint64_t entry_toc = c.gpr[2];
    uint64_t steps = 0;

    uint64_t ban_lr[32] = {};
    int nban = 0, ban_w = 0;

    uint64_t rban[16] = {};
    int nrban = 0, rban_w = 0;

    uint64_t stub_lr_sticky = 0;
    int stub_same_lr = 0;
    uint64_t region_sticky = 0;
    int region_hits = 0;
    uint64_t last_leave = 0;
    int leave_same = 0;
    int walk_guard = 0;

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

    while (!c.halted) {
        // Phase XIII: snap out of nonsense PCs (0 / high kernel-ish)
        if (c.pc == 0 || c.pc >= 0x10000000ull) {
            uint64_t alt = recover_return_pc(c, chunks, n, c.pc, nullptr, 0, nullptr, 0);
            if (!alt && last_good_ret) alt = last_good_ret;
            if (alt) {
                static int wild_log = 0;
                if (wild_log < 12) {
                    ps3rt_log("[ps3] Phase XIII wild-pc 0x%llx -> 0x%llx\n",
                        (unsigned long long)c.pc, (unsigned long long)alt);
                    ++wild_log;
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
            if (spin_count >= 0x1000 && (spin_count & 0xFF) == 0) {
                uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                if (alt && alt != c.pc) {
                    c.pc = alt; c.gpr[3] = 0; spin_count = 0; continue;
                }
            }
        } else {
            spin_pc = c.pc; spin_count = 0;
        }

        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i) {
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }
        }

        if (!hit) {
            // PC left translated range
            uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
            if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
            if (alt && external_escapes < 262144) {
                if (logged < 32) {
                    ps3rt_log("[ps3] Phase XI force-out pc=0x%llx -> 0x%llx\n",
                        (unsigned long long)c.pc, (unsigned long long)alt);
                    ++logged;
                }
                // Detect sequential walk
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
                c.pc = alt;
                c.gpr[3] = 0;
                ++external_escapes;
                continue;
            }
            ps3rt_log("[ps3] halt: PC left range pc=0x%llx lr=0x%llx escapes=%d nrban=%d last_good=0x%llx\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                external_escapes, nrban, (unsigned long long)last_good_ret);
            c.halted = true;
            break;
        }

        // Import / null stub zone
        bool nullish = (c.pc == 0x39800000ull) || ((c.pc & 0xFFFF0000ull) == 0x39800000ull);
        bool importish = nullish;
        if (nullish || (c.pc < 0x10100ull)) {
            uint64_t ret = c.lr;
            if (!ret || is_banned(ret) || !pc_in_chunks(ret, chunks, n))
                ret = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
            if (!ret && last_good_ret && !is_banned(last_good_ret)
                && last_good_ret != failed_escape
                && pc_in_chunks(last_good_ret, chunks, n))
                ret = last_good_ret;

            if (ret && external_escapes < 262144) {
                if (nullish) ps3rt_prx_import_stub(&c, c.pc);
                else c.gpr[3] = 0;

                bool same_site = nullish && last_null_lr &&
                    (ret == last_null_lr ||
                     (ret >= (last_null_lr & ~0xFFull) && ret < (last_null_lr & ~0xFFull) + 0x100));
                if (same_site) {
                    if (++stub_same_lr > 512) {
                        ++soft_breaks;
                        stub_same_lr = 0;
                        uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban, rban, nrban);
                        if (alt && (alt < (ret & ~0xFFull) || alt >= (ret & ~0xFFull) + 0x100))
                            ret = alt;
                        else if (last_good_ret && !is_banned(last_good_ret))
                            ret = last_good_ret;
                        else {
                            uint64_t stride = 0x40;
                            if (soft_breaks >= 4) stride = 0x100;
                            if (soft_breaks >= 12) stride = 0x400;
                            if (soft_breaks >= 24) stride = 0x1000;
                            uint64_t next = (ret & ~0xFull) + stride;
                            if (pc_in_chunks(next, chunks, n)) ret = next;
                            else if (pc_in_chunks(ret + 4, chunks, n)) ret = ret + 4;
                        }
                        if (soft_breaks >= 48) {
                            add_ban(last_null_lr);
                            last_good_ret = 0;
                            last_null_lr = 0;
                        }
                    }
                } else {
                    stub_same_lr = 0;
                    last_null_lr = nullish ? ret : 0;
                }

                if (logged < 96) {
                    ps3rt_log("[ps3] external/import stub pc=0x%llx -> ret=0x%llx (#%d)%s\n",
                        (unsigned long long)c.pc, (unsigned long long)ret,
                        external_escapes + 1,
                        (nullish || importish) ? " [null/import]" : "");
                    ++logged;
                }
                if (ret != last_null_lr) last_good_ret = ret;
                c.pc = ret;
                ++external_escapes;
                continue;
            }

            if (last_good_ret && external_escapes < 262144) {
                if (nullish) ps3rt_prx_import_stub(&c, c.pc);
                else c.gpr[3] = 0;
                c.pc = last_good_ret;
                ++external_escapes;
                continue;
            }

            ps3rt_log("[ps3] halt: PC left range pc=0x%llx lr=0x%llx escapes=%d nrban=%d last_good=0x%llx\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                external_escapes, nrban, (unsigned long long)last_good_ret);
            c.halted = true;
            break;
        }

        hit->fn(c);

        if ((++steps & 0x3FFFFull) == 0) {
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
}
