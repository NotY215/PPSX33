// Phase O: break import-stub ping-pong; restore TOC; force past sticky call sites
static inline uint64_t recover_return_pc(PPUContext& c, const PPUChunk* chunks, size_t n,
                                         uint64_t exclude_pc = 0,
                                         const uint64_t* ban = nullptr, int nban = 0) {
    auto banned = [&](uint64_t p) -> bool {
        if (!p || p == exclude_pc) return true;
        for (int i = 0; i < nban; ++i) if (ban[i] == p) return true;
        return false;
    };
    if (c.ctr && !banned(c.ctr) && (c.ctr & 3ull) == 0 && pc_in_chunks(c.ctr, chunks, n))
        return c.ctr;
    uint64_t sp = c.gpr[1];
    for (int dir = 0; dir < 2; ++dir) {
        int start = (dir == 0) ? 0 : -0x800;
        int end   = (dir == 0) ? 0x3000 : 0;
        for (int off = start; off <= end; off += 8) {
            uint64_t a = sp + (uint64_t)(int64_t)off;
            if (!guest_ok(c, a, 8)) continue;
            uint64_t cand = rd64(c, a);
            if (banned(cand) || (cand & 3ull) != 0) continue;
            if (pc_in_chunks(cand, chunks, n))
                return cand;
        }
    }
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
    uint64_t ban_lr[24] = {};
    int nban = 0;

    // Phase O: import stub ping-pong detector
    uint64_t stub_lr_sticky = 0;
    int stub_same_lr = 0;

    uint64_t spin_pc = 0;
    uint64_t spin_count = 0;
    uint64_t progress_pokes = 0;
    uint64_t last_hb_pc = 0;
    int same_hb = 0;

    auto is_banned = [&](uint64_t p) -> bool {
        for (int i = 0; i < nban; ++i) if (ban_lr[i] == p) return true;
        return false;
    };
    auto add_ban = [&](uint64_t lr) {
        if (!lr || is_banned(lr)) return;
        if (nban < 24) ban_lr[nban++] = lr;
        std::fprintf(stderr, "[ps3] Phase O ban=0x%llx (nban=%d)\n",
            (unsigned long long)lr, nban);
    };

    while (!c.halted) {
        // Restore TOC if corrupted (GOW was setting r2 to garbage like 0x658c0050)
        if (entry_toc != 0) {
            uint64_t r2 = c.gpr[2];
            bool bad_toc = (r2 == 0) || (r2 > 0x10000000ull) || ((r2 & 3ull) != 0);
            if (bad_toc)
                c.gpr[2] = entry_toc;
        }

        bool is_wait = (c.pc != 0 && c.pc == c.lr);
        if (c.pc == spin_pc || is_wait) {
            if (c.pc != spin_pc) { spin_pc = c.pc; spin_count = 0; }
            ++spin_count;
            if ((spin_count & 0x7F) == 0) {
                ps3rt_progress_hw();
                ++progress_pokes;
            }
            if (spin_count >= 0x1000 && (spin_count & 0xFF) == 0) {
                uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban);
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

        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i)
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }

        if (!hit) {
            bool nullish = (c.pc == 0) || (c.pc >= 0x80000000ull && c.pc < 0xA0000000ull)
                        || (c.pc >= 0x90000000ull && c.pc < 0xA0000000ull);
            bool importish = (c.pc == 0x39800000ull) || ((c.pc & 0xFFFF0000ull) == 0x39800000ull);

            // --- Phase O: import stub ping-pong ---
            // Pattern: pc=0x39800000, lr=L, return to L, code at L immediately calls stub again.
            // After a few identical (stub,lr) pairs, force pc = lr+4 and ban lr.
            if (importish && c.lr) {
                if (c.lr == stub_lr_sticky) {
                    ++stub_same_lr;
                } else {
                    stub_lr_sticky = c.lr;
                    stub_same_lr = 1;
                }
                if (stub_same_lr >= 4) {
                    uint64_t force = c.lr + 4;
                    // Prefer a stack candidate if available
                    uint64_t alt = recover_return_pc(c, chunks, n, c.lr, ban_lr, nban);
                    if (alt && alt != c.lr && !is_banned(alt))
                        force = alt;
                    std::fprintf(stderr,
                        "[ps3] Phase O break stub-loop lr=0x%llx count=%d -> 0x%llx\n",
                        (unsigned long long)c.lr, stub_same_lr, (unsigned long long)force);
                    std::fflush(stderr);
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
                ret = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban);

            // If importish and lr is banned, force lr+4
            if (importish && !ret && c.lr) {
                uint64_t force = c.lr + 4;
                if (pc_in_chunks(force, chunks, n))
                    ret = force;
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

                if (!nullish && !importish && !is_banned(ret))
                    last_good_ret = ret;

                if (nullish || (c.lr && (c.lr == last_null_lr || is_banned(c.lr)))) {
                    ++soft_breaks;
                    if (c.lr) add_ban(c.lr);

                    if (failed_escape && soft_breaks >= 2) {
                        add_ban(failed_escape);
                        if (last_good_ret == failed_escape) last_good_ret = 0;
                        failed_escape = 0;
                    }

                    uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban);

                    if (soft_breaks > 4 && alt && alt != ret && !is_banned(alt)) {
                        std::fprintf(stderr, "[ps3] Phase O stack-escape -> 0x%llx (soft=%d)\n",
                            (unsigned long long)alt, soft_breaks);
                        ret = alt;
                        soft_breaks = 2;
                        last_null_lr = 0;
                    } else if (soft_breaks > 6 && last_good_ret && !is_banned(last_good_ret)
                               && last_good_ret != failed_escape && last_good_ret != c.lr) {
                        std::fprintf(stderr, "[ps3] Phase O escape -> last_good=0x%llx\n",
                            (unsigned long long)last_good_ret);
                        failed_escape = last_good_ret;
                        ret = last_good_ret;
                        ++escape_fails;
                        soft_breaks = 0;
                        last_null_lr = 0;
                        if (escape_fails >= 2) {
                            add_ban(last_good_ret);
                            last_good_ret = 0;
                            escape_fails = 0;
                            uint64_t alt2 = recover_return_pc(c, chunks, n, ret, ban_lr, nban);
                            if (alt2) ret = alt2;
                        }
                    } else if (soft_breaks > 10) {
                        if (alt) ret = alt;
                        else if (ret) {
                            ret = ret + 0x1000;
                            if (!pc_in_chunks(ret, chunks, n))
                                ret = recover_return_pc(c, chunks, n, 0, ban_lr, nban);
                        }
                        std::fprintf(stderr, "[ps3] Phase O hard-escape ret=0x%llx\n",
                            (unsigned long long)ret);
                        soft_breaks = 0;
                        last_null_lr = 0;
                        if (!ret) { c.halted = true; break; }
                    } else {
                        uint64_t stride = (soft_breaks > 3) ? 0x100u : 0x40u;
                        if (ret && pc_in_chunks(ret + stride, chunks, n)) {
                            if (soft_breaks <= 8)
                                std::fprintf(stderr, "[ps3] null-loop skip: 0x%llx +0x%llx -> 0x%llx (soft=%d)\n",
                                    (unsigned long long)ret, (unsigned long long)stride,
                                    (unsigned long long)(ret + stride), soft_breaks);
                            ret = ret + stride;
                        }
                    }
                    last_null_lr = c.lr;
                }

                if (!ret) { c.halted = true; break; }
                c.pc = ret;
                ++external_escapes;
                continue;
            }

            {
                uint64_t alt = recover_return_pc(c, chunks, n, last_null_lr, ban_lr, nban);
                if (alt && external_escapes < 2000000) {
                    if (importish || nullish)
                        ps3rt_prx_import_stub(&c, c.pc);
                    c.pc = alt;
                    if (!nullish) last_good_ret = alt;
                    ++external_escapes;
                    continue;
                }
            }

            std::fprintf(stderr,
                "[ps3] halt: PC left range\n"
                "  pc=0x%llx lr=0x%llx r1=0x%llx r2=0x%llx escapes=%d last_good=0x%llx soft=%d nban=%d\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                (unsigned long long)c.gpr[1], (unsigned long long)c.gpr[2],
                external_escapes, (unsigned long long)last_good_ret, soft_breaks, nban);
            c.halted = true;
            break;
        }

        hit->fn(c);

        if ((++steps & 0x3FFFull) == 0) {
            ps3rt_progress_hw();
            ++progress_pokes;
            // If heartbeats keep showing import stub PC, force break
            if (c.pc == 0x39800000ull || (c.pc & 0xFFFF0000ull) == 0x39800000ull) {
                uint64_t force = (c.lr && !is_banned(c.lr)) ? (c.lr + 4) : 0;
                uint64_t alt = recover_return_pc(c, chunks, n, c.lr, ban_lr, nban);
                if (alt) force = alt;
                if (force && pc_in_chunks(force, chunks, n)) {
                    std::fprintf(stderr, "[ps3] Phase O hb-break stub -> 0x%llx\n",
                        (unsigned long long)force);
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
                    uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban);
                    if (alt && alt != c.pc && !is_banned(alt)) {
                        add_ban(c.pc);
                        c.pc = alt; c.gpr[3] = 0;
                        spin_count = 0; spin_pc = 0; same_hb = 0;
                    } else if (same_hb >= 3) {
                        c.pc += 4; c.gpr[3] = 0;
                        spin_count = 0; spin_pc = 0; same_hb = 0;
                    }
                }
            } else same_hb = 0;
            last_hb_pc = c.pc;
            std::fprintf(stderr, "[ps3] heartbeat steps=%llu pc=0x%llx lr=0x%llx r2=0x%llx pokes=%llu nban=%d\n",
                (unsigned long long)steps, (unsigned long long)c.pc,
                (unsigned long long)c.lr, (unsigned long long)c.gpr[2],
                (unsigned long long)progress_pokes, nban);
            std::fflush(stderr);
        }
    }
}
