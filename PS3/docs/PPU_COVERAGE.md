# PPU instruction coverage (`src/core/ppu_lifter.cpp: lift_one`)

Primary opcode -> status. "Rc" = also sets CR0 when the record bit is set.

| Opcode | Mnemonics | Notes |
|--------|-----------|-------|
| 10 / 11 | cmpli / cmpi | 32/64-bit via L bit |
| 14 / 15 | addi, li / addis, lis | |
| 16 | bc (+ bdnz etc.), AA/LK | `bc_taken()` in runtime |
| 17 | sc | `ps3rt_syscall` |
| 18 | b, bl, ba, bla | |
| 19 | bclr (blr, bltlr...), bcctr (bctr, bctrl) | xo 16 / 528 |
| 21 | rlwinm (slwi, srwi, clrlwi, ...) | Rc supported |
| 24-29 | ori, oris, xori, xoris, andi., andis. | |
| 31 | add, subf, or (mr), and, xor, cmp, cmpl, lwzx, stwx, mfspr/mtspr (LR, CTR, XER) | xo 266,40,444,28,316,0,32,23,151,339,467; Rc supported |
| 32-34, 36-38, 40, 44 | lwz, lwzu, lbz, stw, stwu, stb, lhz, sth | |
| 58 / 62 | ld, ldu, lwa / std, stdu | DS-form |

Everything else is emitted as `ps3rt_unimplemented()` and counted in `codebase/lift_report.txt`.
Not modeled: FPU, VMX, atomics, XER carry/overflow, CR summary-overflow, trap instructions, cache ops.
