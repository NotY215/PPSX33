// Phase J: force-break pc==lr wait loops + stronger HW progress
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
        int start = (dir == 0) ? 0 : -0x400;
        int end   = (dir == 0) ? 0x2000 : 0;
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
    uint64_t entry_toc = c.gpr[2];
    uint64_t steps = 0;
    uint64_t ban_lr[8] = {};
    int nban = 0;

    // Phase J spin / wait tracking
    uint64_t spin_pc = 0;
    uint64_t spin_count = 0;
    uint64_t progress_pokes = 0;
    uint64_t last_hb_pc = 0;
    int same_hb = 0;

    auto add_ban = [&](uint64_t lr) {
        if (!lr) return;
        for (int i = 0; i < nban; ++i) if (ban_lr[i] == lr) return;
        if (nban < 8) ban_lr[nban++] = lr;
        std::fprintf(stderr, "[ps3] Phase J ban LR=0x%llx (nban=%d)\n",
            (unsigned long long)lr, nban);
    };

    while (!c.halted) {
        if (c.gpr[2] == 0 && entry_toc != 0)
            c.gpr[2] = entry_toc;

        // Detect same-PC spin (including pc==lr wait loops)
        bool is_wait = (c.pc != 0 && c.pc == c.lr);
        if (c.pc == spin_pc || is_wait) {
            if (c.pc != spin_pc) { spin_pc = c.pc; spin_count = 0; }
            ++spin_count;
            // Poke HW every 256 iterations (was 4096)
            if ((spin_count & 0xFF) == 0) {
                ps3rt_progress_hw();
                ++progress_pokes;
                if ((progress_pokes & 0x1F) == 1) {
                    std::fprintf(stderr,
                        "[ps3] Phase J spin pc=0x%llx count=%llu pokes=%llu wait=%d\n",
                        (unsigned long long)spin_pc,
                        (unsigned long long)spin_count,
                        (unsigned long long)progress_pokes,
                        is_wait ? 1 : 0);
                    std::fflush(stderr);
                }
            }
            // Force break after ~16k same-PC iterations
            if (spin_count >= 0x4000 && (spin_count & 0x3FF) == 0) {
                uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban);
                if (alt && alt != c.pc) {
                    std::fprintf(stderr, "[ps3] Phase J break spin via alt=0x%llx\n",
                        (unsigned long long)alt);
                    add_ban(c.pc);
                    c.pc = alt;
                    spin_count = 0;
                    spin_pc = 0;
                    continue;
                }
                // No alt: advance PC by 4 (skip current wait insn)
                if (is_wait && spin_count >= 0x10000) {
                    std::fprintf(stderr, "[ps3] Phase J force +4 on wait pc=0x%llx\n",
                        (unsigned long long)c.pc);
                    c.pc += 4;
                    c.gpr[3] = 0; // pretend success
                    spin_count = 0;
                    spin_pc = 0;
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

            uint64_t ret = 0;
            if (c.lr && c.lr != c.pc && pc_in_chunks(c.lr, chunks, n)) {
                bool banned = false;
                for (int i = 0; i < nban; ++i) if (ban_lr[i] == c.lr) { banned = true; break; }
                if (!banned) ret = c.lr;
            }
            if (!ret && last_good_ret && pc_in_chunks(last_good_ret, chunks, n))
                ret = last_good_ret;
            if (!ret)
                ret = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban);

            if (ret && external_escapes < 1048576) {
                if (importish || nullish)
                    ps3rt_prx_import_stub(&c, c.pc);
                else
                    c.gpr[3] = 0;

                if (logged < 64) {
                    std::fprintf(stderr,
                        "[ps3] external/import stub pc=0x%llx -> ret=0x%llx (#%d)%s\n",
                        (unsigned long long)c.pc, (unsigned long long)ret,
                        external_escapes + 1,
                        (nullish || importish) ? " [null/import]" : "");
                    ++logged;
                }
                {
                    bool banned = false;
                    for (int i = 0; i < nban; ++i) if (ban_lr[i] == ret) { banned = true; break; }
                    if (!banned && ret != last_null_lr)
                        last_good_ret = ret;
                }

                // null-loop soft skip
                if (nullish || (c.lr && c.lr == last_null_lr)) {
                    ++soft_breaks;
                    uint64_t stride = 0x40;
                    if (soft_breaks > 12) stride = 0x100;
                    if (soft_breaks > 24) stride = 0x400;
                    if (soft_breaks > 36) stride = 0x1000;
                    if (soft_breaks > 48) {
                        add_ban(c.lr ? c.lr : ret);
                        uint64_t alt = recover_return_pc(c, chunks, n, ret, ban_lr, nban);
                        if (alt) ret = alt;
                    }
                    if (ret && soft_breaks <= 64) {
                        uint64_t new_pc = ret + stride;
                        if (pc_in_chunks(new_pc, chunks, n)) {
                            if (soft_breaks <= 40)
                                std::fprintf(stderr, "[ps3] null-loop skip: 0x%llx +0x%llx -> 0x%llx (soft_breaks=%d)\n",
                                    (unsigned long long)ret, (unsigned long long)stride,
                                    (unsigned long long)new_pc, soft_breaks);
                            ret = new_pc;
                        }
                    }
                    last_null_lr = c.lr;
                }

                c.pc = ret;
                ++external_escapes;
                continue;
            }

            {
                uint64_t alt = recover_return_pc(c, chunks, n, last_null_lr, ban_lr, nban);
                if (alt && external_escapes < 1048576) {
                    std::fprintf(stderr, "[ps3] deep-recover alt=0x%llx escapes=%d\n",
                        (unsigned long long)alt, external_escapes);
                    if (importish || nullish)
                        ps3rt_prx_import_stub(&c, c.pc);
                    c.pc = alt;
                    last_good_ret = alt;
                    ++external_escapes;
                    continue;
                }
            }

            std::fprintf(stderr,
                "[ps3] halt: PC left range with no usable return\n"
                "  pc=0x%llx lr=0x%llx ctr=0x%llx r1=0x%llx r2=0x%llx escapes=%d last_good=0x%llx soft_breaks=%d nban=%d\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                (unsigned long long)c.ctr, (unsigned long long)c.gpr[1],
                (unsigned long long)c.gpr[2], external_escapes,
                (unsigned long long)last_good_ret, soft_breaks, nban);
            c.halted = true;
            break;
        }

        hit->fn(c);

        // Heartbeat every 64K outer steps (more frequent)
        if ((++steps & 0xFFFFull) == 0) {
            ps3rt_progress_hw();
            ++progress_pokes;

            // Phase J: consecutive same pc==lr heartbeats => force break
            if (c.pc != 0 && c.pc == c.lr && c.pc == last_hb_pc) {
                ++same_hb;
                if (same_hb >= 2) {
                    uint64_t alt = recover_return_pc(c, chunks, n, c.pc, ban_lr, nban);
                    if (alt && alt != c.pc) {
                        std::fprintf(stderr, "[ps3] Phase J hb-break spin via alt=0x%llx (same_hb=%d)\n",
                            (unsigned long long)alt, same_hb);
                        add_ban(c.pc);
                        c.pc = alt;
                        c.gpr[3] = 0;
                        spin_count = 0;
                        spin_pc = 0;
                        same_hb = 0;
                    } else if (same_hb >= 4) {
                        std::fprintf(stderr, "[ps3] Phase J hb-force +4 pc=0x%llx (same_hb=%d)\n",
                            (unsigned long long)c.pc, same_hb);
                        c.pc += 4;
                        c.gpr[3] = 0;
                        spin_count = 0;
                        spin_pc = 0;
                        same_hb = 0;
                    }
                }
            } else {
                same_hb = 0;
            }
            last_hb_pc = c.pc;

            std::fprintf(stderr, "[ps3] heartbeat steps=%llu pc=0x%llx lr=0x%llx r2=0x%llx pokes=%llu\n",
                (unsigned long long)steps, (unsigned long long)c.pc,
                (unsigned long long)c.lr, (unsigned long long)c.gpr[2],
                (unsigned long long)progress_pokes);
            std::fflush(stderr);
        }
    }
}
