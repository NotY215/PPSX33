// Phase XV: import stub before force-out; wild-PC excludes 0x39800000
// Phase XIV: cycle-break ring for short PC loops (GOW 0/0x331c30/0x3ca524)
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

    uint64_t hist[8] = {};
    int hist_n = 0;
    int cycle_hits = 0;

    while (!c.halted) {
        if (ps3rt_should_exit()) {
            ps3rt_log("[ps3] exit requested (window closed)\n");
            c.halted = true;
            break;
        }

        if (hist_n == 0 || hist[(hist_n - 1) & 7] != c.pc) {
            hist[hist_n & 7] = c.pc;
            ++hist_n;
        }
        if (hist_n >= 12) {
            uint64_t a = hist[(hist_n - 1) & 7];
            uint64_t b = hist[(hist_n - 2) & 7];
            uint64_t d = hist[(hist_n - 3) & 7];
            bool cycle2 = (a == hist[(hist_n - 3) & 7] && b == hist[(hist_n - 4) & 7] && a != b);
            bool cycle3 = (a == hist[(hist_n - 4) & 7] && b == hist[(hist_n - 5) & 7] && d == hist[(hist_n - 6) & 7]
                           && a != b && b != d);
            if (cycle2 || cycle3) {
                ++cycle_hits;
                ps3rt_progress_hw();
                ++progress_pokes;
                if ((cycle_hits % 64) == 1) {
                    ps3rt_log("[ps3] Phase XIV cycle-break pc=0x%llx hits=%d\n",
                        (unsigned long long)c.pc, cycle_hits);
                }
                uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
                if (alt && alt != c.pc && alt != a && alt != b) {
                    c.pc = alt; c.gpr[3] = 0; continue;
                }
                const PPUChunk* ch = nullptr;
                for (size_t i = 0; i < n; ++i)
                    if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { ch = &chunks[i]; break; }
                if (ch && c.pc + 4 < ch->end) { c.pc += 4; c.gpr[3] = 0; continue; }
                if (last_good_ret) {
                    uint64_t stride = (cycle_hits > 256) ? 0x1000ull : 0x100ull;
                    uint64_t nx = (last_good_ret & ~0xFull) + stride;
                    if (pc_in_chunks(nx, chunks, n)) { c.pc = nx; c.gpr[3] = 0; continue; }
                }
            } else {
                cycle_hits = 0;
            }
        }

        if ((c.pc == 0 || c.pc >= 0x10000000ull) &&
            !(c.pc == 0x39800000ull || (c.pc & 0xFFFF0000ull) == 0x39800000ull)) {
            uint64_t alt = recover_return_pc(c, chunks, n, c.pc, nullptr, 0, nullptr, 0);
            if (!alt && last_good_ret) alt = last_good_ret;
            if (alt) {
                static int wild_log = 0;
                if (wild_log < 12) {
                    ps3rt_log("[ps3] Phase XIV wild-pc 0x%llx -> 0x%llx\n",
                        (unsigned long long)c.pc, (unsigned long long)alt);
                    ++wild_log;
                }
                c.pc = alt; c.gpr[3] = 0; continue;
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
                        if (++stub_same_lr > 64) {
                            ++soft_breaks;
                            stub_same_lr = 0;
                            uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban, rban, nrban);
                            if (alt && alt != ret) ret = alt;
                            else {
                                uint64_t stride = (soft_breaks >= 6) ? 0x1000ull : 0x400ull;
                                uint64_t next = (ret & ~0xFull) + stride;
                                if (pc_in_chunks(next, chunks, n)) ret = next;
                                else if (pc_in_chunks(ret + 4, chunks, n)) ret = ret + 4;
                            }
                            if (soft_breaks >= 8) { add_ban(last_null_lr); last_null_lr = 0; }
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
