// Phase XI: never walk-ban the image; sticky leave prefers high last_good / stack
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
    // Prefer stack slots that look like mid/high code (GOW lives ~0x10000-0x500000)
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
            // Prefer addresses in the "active" band seen in successful boots
            if (cand >= 0x300000ull && cand < 0x500000ull) return cand;
            if (cand >= 0x20000ull && !best_hi) best_hi = cand;
            if (!best_hi) best_hi = cand;
        }
    }
    if (best_hi) return best_hi;
    return 0; // Phase XI: do NOT fall back to sequential chunk starts (caused ban-walk)
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
    int walk_guard = 0; // Phase XI: detect sequential leave walk

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
        // Phase XI: never region-ban low image walk targets (< 0x100000)
        // Those are code pages; banning them causes the 0x18000..0x90000 walk.
        uint64_t b = base & ~0x3FFull;
        if (b < 0x100000ull) return;
        if (in_rban(b)) return;
        rban[rban_w] = b;
        rban_w = (rban_w + 1) % 16;
        if (nrban < 16) ++nrban;
        std::fprintf(stderr, "[ps3] Phase XI region-ban 0x%llx (nrban=%d)\n",
            (unsigned long long)b, nrban);
    };

    while (!c.halted) {
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
                if (alt && alt != c.pc && !is_banned(alt)) {
                    add_ban(c.pc);
                    c.pc = alt;
                    spin_count = 0; spin_pc = 0;
                    continue;
                }
                if (is_wait && spin_count >= 0x4000) {
                    c.pc += 4; c.gpr[3] = 0;
                    spin_count = 0; spin_pc = 0;
                    continue;
                }
            }
        } else {
            spin_pc = c.pc;
            spin_count = 1;
        }

        // Only force-out of banned regions in the HIGH sticky zone, not low image
        if (in_rban(c.pc) && c.pc >= 0x100000ull) {
            uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
            if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
            if (alt) {
                static int rban_log = 0;
                if (rban_log < 8) {
                    std::fprintf(stderr, "[ps3] Phase XI force-out pc=0x%llx -> 0x%llx\n",
                        (unsigned long long)c.pc, (unsigned long long)alt);
                    ++rban_log;
                }
                c.pc = alt;
                c.gpr[3] = 0;
                continue;
            }
        }

        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i)
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }

        if (!hit) {
            bool nullish = (c.pc == 0) || (c.pc >= 0x80000000ull && c.pc < 0xA0000000ull);
            bool importish = (c.pc == 0x39800000ull) || ((c.pc & 0xFFFF0000ull) == 0x39800000ull);
            if (c.pc >= 0x0F000000ull && c.pc < 0x10000000ull) nullish = true;
            if (c.pc >= 0x600000ull && c.pc < 0x700000ull) nullish = true;
            // Garbage PCs
            if (c.pc > 0x10000000ull && !importish) nullish = true;

            if (importish && c.lr) {
                if (c.lr == stub_lr_sticky) ++stub_same_lr;
                else { stub_lr_sticky = c.lr; stub_same_lr = 1; }
                if (stub_same_lr >= 3) {
                    uint64_t force = c.lr + 4;
                    uint64_t alt = recover_return_pc(c, chunks, n, c.lr, ban_lr, nban, rban, nrban);
                    if (alt && !is_banned(alt)) force = alt;
                    else if (last_good_ret && !is_banned(last_good_ret)) force = last_good_ret;
                    ps3rt_prx_import_stub(&c, c.pc);
                    c.gpr[3] = 0;
                    add_ban(c.lr);
                    c.pc = force;
                    stub_same_lr = 0;
                    stub_lr_sticky = 0;
                    ++external_escapes;
                    continue;
                }
            } else if (!importish) {
                stub_same_lr = 0;
                stub_lr_sticky = 0;
            }

            uint64_t ret = 0;
            if (c.lr && c.lr != c.pc && !is_banned(c.lr) && pc_in_chunks(c.lr, chunks, n))
                ret = c.lr;
            if (!ret && last_good_ret && !is_banned(last_good_ret)
                && last_good_ret != failed_escape
                && pc_in_chunks(last_good_ret, chunks, n))
                ret = last_good_ret;
            if (!ret)
                ret = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);

            if (importish && !ret && c.lr) {
                uint64_t force = c.lr + 4;
                if (pc_in_chunks(force, chunks, n) && !is_banned(force))
                    ret = force;
                else if (!is_banned(c.lr) && pc_in_chunks(c.lr, chunks, n))
                    ret = c.lr;
            }

            if (ret && external_escapes < 2000000) {
                if (importish || nullish)
                    ps3rt_prx_import_stub(&c, c.pc);
                else
                    c.gpr[3] = 0;

                if (logged < 40) {
                    std::fprintf(stderr,
                        "[ps3] external/import stub pc=0x%llx -> ret=0x%llx (#%d)%s\n",
                        (unsigned long long)c.pc, (unsigned long long)ret,
                        external_escapes + 1,
                        (nullish || importish) ? " [null/import]" : "");
                    ++logged;
                }

                // Track last_good only for solid mid/high code returns
                if (!nullish && !importish && !is_banned(ret) && ret >= 0x20000ull)
                    last_good_ret = ret;

                uint64_t rbase = ret & ~0x3FFull;
                if (rbase == region_sticky) ++region_hits;
                else { region_sticky = rbase; region_hits = 1; }

                // Sticky only for HIGH regions — low image is never region-banned
                if (region_hits >= 4 && rbase >= 0x100000ull) {
                    add_rban(rbase);
                    region_hits = 0;
                    uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban, rban, nrban);
                    if (alt && alt == last_leave) alt = 0;
                    if (!alt && last_good_ret && !is_banned(last_good_ret)) alt = last_good_ret;
                    if (alt) ret = alt;

                    // Walk detector: leaves advancing by ~0x8000 repeatedly
                    if (last_leave && ret > last_leave && (ret - last_leave) <= 0x10000ull) {
                        ++walk_guard;
                    } else {
                        walk_guard = 0;
                    }
                    if (walk_guard >= 3) {
                        // Abort walk — snap back to last_good or a known-good mid address
                        if (last_good_ret && last_good_ret >= 0x300000ull)
                            ret = last_good_ret;
                        else
                            ret = 0x33b1a0; // was a successful leave target in logs
                        if (!pc_in_chunks(ret, chunks, n))
                            ret = recover_return_pc(c, chunks, n, 0, ban_lr, nban, rban, nrban);
                        walk_guard = 0;
                        soft_breaks = 0;
                        std::fprintf(stderr, "[ps3] Phase XI abort address-walk -> 0x%llx\n",
                            (unsigned long long)ret);
                    }

                    last_leave = ret;
                    soft_breaks = 0;
                    std::fprintf(stderr, "[ps3] Phase XI leave sticky -> 0x%llx\n",
                        (unsigned long long)ret);
                } else if (region_hits >= 6 && rbase < 0x100000ull) {
                    // Low sticky: just stride out, do not ban
                    region_hits = 0;
                    if (last_good_ret && !is_banned(last_good_ret))
                        ret = last_good_ret;
                    else
                        ret = (ret + 0x400) & ~3ull;
                    soft_breaks = 0;
                }

                if (nullish || (c.lr && (c.lr == last_null_lr || is_banned(c.lr)))) {
                    ++soft_breaks;
                    if (c.lr && c.lr >= 0x20000ull) add_ban(c.lr);

                    uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban, rban, nrban);

                    if (soft_breaks > 3 && alt && !is_banned(alt)) {
                        ret = alt;
                        soft_breaks = 1;
                        last_null_lr = 0;
                    } else if (soft_breaks > 6) {
                        if (last_good_ret && !is_banned(last_good_ret))
                            ret = last_good_ret;
                        else if (alt)
                            ret = alt;
                        soft_breaks = 0;
                        last_null_lr = 0;
                        if (!ret) { c.halted = true; break; }
                    } else {
                        uint64_t stride = (soft_breaks > 2) ? 0x100u : 0x40u;
                        if (ret && !is_banned(ret + stride) && pc_in_chunks(ret + stride, chunks, n))
                            ret = ret + stride;
                    }
                    last_null_lr = c.lr;
                }

                if (!ret || is_banned(ret)) {
                    uint64_t alt = recover_return_pc(c, chunks, n, 0, ban_lr, nban, rban, nrban);
                    if (alt) ret = alt;
                    else if (last_good_ret) ret = last_good_ret;
                    if (!ret) { c.halted = true; break; }
                }
                c.pc = ret;
                ++external_escapes;
                continue;
            }

            uint64_t alt = recover_return_pc(c, chunks, n, last_null_lr, ban_lr, nban, rban, nrban);
            if (!alt && last_good_ret) alt = last_good_ret;
            if (alt && external_escapes < 2000000) {
                if (importish || nullish) ps3rt_prx_import_stub(&c, c.pc);
                c.pc = alt;
                ++external_escapes;
                continue;
            }

            if (importish && c.lr && (c.lr & 3ull) == 0 && c.lr >= 0x10100ull) {
                ps3rt_prx_import_stub(&c, c.pc);
                c.gpr[3] = 0;
                c.pc = is_banned(c.lr) ? (c.lr + 4) : c.lr;
                ++external_escapes;
                continue;
            }

            std::fprintf(stderr,
                "[ps3] halt: PC left range pc=0x%llx lr=0x%llx escapes=%d nrban=%d last_good=0x%llx\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr, external_escapes, nrban,
                (unsigned long long)last_good_ret);
            c.halted = true;
            break;
        }

        // Remember solid execution PCs as last_good
        if (c.pc >= 0x300000ull && c.pc < 0x500000ull)
            last_good_ret = c.pc;

        hit->fn(c);

        if ((++steps & 0x3FFFull) == 0) {
            ps3rt_progress_hw();
            ++progress_pokes;
            if (c.pc == 0x39800000ull || (c.pc & 0xFFFF0000ull) == 0x39800000ull) {
                uint64_t force = (c.lr && !is_banned(c.lr)) ? (c.lr + 4) : 0;
                uint64_t alt = recover_return_pc(c, chunks, n, c.lr, ban_lr, nban, rban, nrban);
                if (alt) force = alt;
                else if (last_good_ret) force = last_good_ret;
                if (force && pc_in_chunks(force, chunks, n)) {
                    if (c.lr) add_ban(c.lr);
                    ps3rt_prx_import_stub(&c, c.pc);
                    c.gpr[3] = 0;
                    c.pc = force;
                    stub_same_lr = 0;
                }
            }
            if (c.pc != 0 && c.pc == c.lr && c.pc == last_hb_pc) {
                ++same_hb;
                if (same_hb >= 2) {
                    uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban, rban, nrban);
                    if (alt && !is_banned(alt)) {
                        c.pc = alt; c.gpr[3] = 0;
                        spin_count = 0; same_hb = 0;
                    } else if (same_hb >= 3) {
                        c.pc += 4; c.gpr[3] = 0;
                        spin_count = 0; same_hb = 0;
                    }
                }
            } else same_hb = 0;
            last_hb_pc = c.pc;
            std::fprintf(stderr, "[ps3] heartbeat steps=%llu pc=0x%llx lr=0x%llx r2=0x%llx pokes=%llu nrban=%d\n",
                (unsigned long long)steps, (unsigned long long)c.pc,
                (unsigned long long)c.lr, (unsigned long long)c.gpr[2],
                (unsigned long long)progress_pokes, nrban);
            std::fflush(stderr);
        }
    }
}
