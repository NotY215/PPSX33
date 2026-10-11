// Recover a return address when LR is dead: try CTR, then scan stack near r1
// (wider window + both directions). Prefer code PCs that live inside lifted chunks.
// exclude_pc: skip this candidate (used to break out of a known looping LR).
static inline uint64_t recover_return_pc(PPUContext& c, const PPUChunk* chunks, size_t n,
                                         uint64_t exclude_pc = 0) {
    if (c.ctr && c.ctr != c.pc && c.ctr != exclude_pc &&
        (c.ctr & 3ull) == 0 && pc_in_chunks(c.ctr, chunks, n))
        return c.ctr;
    uint64_t sp = c.gpr[1];
    for (int dir = 0; dir < 2; ++dir) {
        int start = (dir == 0) ? 0 : -0x200;
        int end   = (dir == 0) ? 0x1000 : 0;
        for (int off = start; off <= end; off += 8) {
            uint64_t a = sp + (uint64_t)(int64_t)off;
            if (!guest_ok(c, a, 8)) continue;
            uint64_t cand = rd64(c, a);
            if (!cand || cand == c.pc || cand == exclude_pc || (cand & 3ull) != 0) continue;
            if (pc_in_chunks(cand, chunks, n))
                return cand;
        }
    }
    return 0;
}

static inline void ppu_run(PPUContext& c, const PPUChunk* chunks, size_t n){
    int external_escapes = 0;
    int logged = 0;
    int same_lr_null = 0;
    int soft_breaks = 0;
    uint64_t last_null_lr = 0;
    uint64_t last_good_ret = 0;
    uint64_t entry_toc = c.gpr[2];
    uint64_t steps = 0;
    while (!c.halted) {
        if (c.gpr[2] == 0 && entry_toc != 0)
            c.gpr[2] = entry_toc;

        const PPUChunk* hit = nullptr;
        for (size_t i = 0; i < n; ++i)
            if (c.pc >= chunks[i].start && c.pc < chunks[i].end) { hit = &chunks[i]; break; }
        if (!hit) {
            bool nullish = (c.pc == 0) || (c.pc >= 0x80000000ull && c.pc < 0xA0000000ull)
                        || (c.pc >= 0x90000000ull && c.pc < 0xA0000000ull);
            bool importish = (c.pc == 0x39800000ull) || ((c.pc & 0xFFFF0000ull) == 0x39800000ull);

            uint64_t ret = 0;
            if (c.lr && c.lr != c.pc && pc_in_chunks(c.lr, chunks, n))
                ret = c.lr;
            else
                ret = recover_return_pc(c, chunks, n);

            if (!ret && last_good_ret && external_escapes < 262144)
                ret = last_good_ret;

            if (ret && external_escapes < 262144) {
                bool same_site = nullish && last_null_lr &&
                    (ret == last_null_lr ||
                     (ret >= (last_null_lr & ~0xFFull) && ret < (last_null_lr & ~0xFFull) + 0x100));
                if (same_site) {
                    if (++same_lr_null > 512) {
                        ++soft_breaks;
                        same_lr_null = 0;
                        uint64_t alt = recover_return_pc(c, chunks, n, ret);
                        if (alt && (alt < (ret & ~0xFFull) || alt >= (ret & ~0xFFull) + 0x100)) {
                            std::fprintf(stderr,
                                "[ps3] null-loop break: lr=0x%llx -> alt=0x%llx (soft_breaks=%d)\n",
                                (unsigned long long)ret, (unsigned long long)alt, soft_breaks);
                            ret = alt;
                            last_null_lr = 0;
                            last_good_ret = alt;
                        } else {
                            uint64_t stride = 0x40;
                            if (soft_breaks >= 4) stride = 0x100;
                            if (soft_breaks >= 12) stride = 0x400;
                            if (soft_breaks >= 24) stride = 0x1000;
                            uint64_t next = (ret & ~0xFull) + stride;
                            if (pc_in_chunks(next, chunks, n)) {
                                std::fprintf(stderr,
                                    "[ps3] null-loop skip: 0x%llx +0x%llx -> 0x%llx (soft_breaks=%d)\n",
                                    (unsigned long long)ret, (unsigned long long)stride,
                                    (unsigned long long)next, soft_breaks);
                                ret = next;
                            } else if (pc_in_chunks(ret + 4, chunks, n)) {
                                ret = ret + 4;
                            }
                            last_null_lr = 0;
                        }
                        if (soft_breaks >= 6)
                            last_good_ret = ret;
                        if (soft_breaks >= 48) {
                            last_good_ret = 0;
                            last_null_lr = 0;
                            std::fprintf(stderr,
                                "[ps3] null-loop region exhausted (soft_breaks=%d) -- dropping sticky\n",
                                soft_breaks);
                        }
                    }
                } else {
                    same_lr_null = 0;
                    last_null_lr = nullish ? ret : 0;
                }

                if (importish || nullish)
                    ps3rt_prx_import_stub(&c, c.pc);
                else
                    c.gpr[3] = 0;

                if (logged < 96) {
                    std::fprintf(stderr,
                        "[ps3] external/import stub pc=0x%llx -> ret=0x%llx (#%d)%s\n",
                        (unsigned long long)c.pc, (unsigned long long)ret,
                        external_escapes + 1,
                        (nullish || importish) ? " [null/import]" : "");
                    ++logged;
                }
                if (ret != last_null_lr)
                    last_good_ret = ret;
                c.pc = ret;
                ++external_escapes;
                continue;
            }

            if (last_good_ret && external_escapes < 262144) {
                std::fprintf(stderr,
                    "[ps3] soft-continue via last_good=0x%llx (pc=0x%llx lr=0x%llx escapes=%d)\n",
                    (unsigned long long)last_good_ret,
                    (unsigned long long)c.pc, (unsigned long long)c.lr,
                    external_escapes);
                if (importish || nullish)
                    ps3rt_prx_import_stub(&c, c.pc);
                else
                    c.gpr[3] = 0;
                c.pc = last_good_ret;
                ++external_escapes;
                continue;
            }

            std::fprintf(stderr,
                "[ps3] halt: PC left range with no usable return\n"
                "  pc=0x%llx lr=0x%llx ctr=0x%llx r1=0x%llx r2=0x%llx escapes=%d last_good=0x%llx soft_breaks=%d\n",
                (unsigned long long)c.pc, (unsigned long long)c.lr,
                (unsigned long long)c.ctr, (unsigned long long)c.gpr[1],
                (unsigned long long)c.gpr[2], external_escapes,
                (unsigned long long)last_good_ret, soft_breaks);
            c.halted = true;
            break;
        }
        hit->fn(c);
        if ((++steps & 0x3FFFFull) == 0) {
            std::fprintf(stderr, "[ps3] heartbeat steps=%llu pc=0x%llx lr=0x%llx r2=0x%llx\n",
                (unsigned long long)steps, (unsigned long long)c.pc,
                (unsigned long long)c.lr, (unsigned long long)c.gpr[2]);
            std::fflush(stderr);
        }
    }
}
