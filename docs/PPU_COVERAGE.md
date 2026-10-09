# PPU instruction coverage (`src/core/ppu_lifter.cpp: lift_one`)

Primary opcode → status. "Rc" = also sets CR0 when the record bit is set.

## Implemented

| Opcode | Mnemonics | Notes |
|--------|-----------|-------|
| 7 | mulli | |
| 8 | subfic | CA not modeled |
| 10 / 11 | cmpli / cmpi | 32/64-bit via L bit |
| 12 / 13 | addic / addic. | CA not modeled |
| 14 / 15 | addi, li / addis, lis | |
| 16 | bc (+ bdnz etc.), AA/LK | `bc_taken()` in runtime |
| 17 | sc | `ps3rt_syscall` |
| 18 | b, bl, ba, bla | |
| 19 | bclr, bcctr, isync, sync, eieio, mcrf, crand/cror/crxor/… | CR logicals + barriers as nop |
| 21 | rlwinm (slwi, srwi, clrlwi, …) | Rc supported |
| 24-29 | ori, oris, xori, xoris, andi., andis. | |
| 30 | rldicl, rldicr, rldic, rldimi | 64-bit rotates (Cell-critical) |
| 31 | See detailed list below | |
| 32-35 | lwz, lwzu, lbz, lbzu | |
| 36-39 | stw, stwu, stb, stbu | |
| 40-43 | lhz, lhzu, lha, lhau | |
| 44-45 | sth, sthu | |
| 46 / 47 | lmw / stmw | |
| 58 / 62 | ld, ldu, lwa / std, stdu | DS-form |

### Opcode 31 (X-form) – implemented XO values

| XO | Mnemonic |
|----|----------|
| 0, 32 | cmp, cmpl |
| 8, 10, 40, 104, 136, 138, 200, 202, 232, 234, 266 | subfc, addc, subf, neg, subfe, adde, subfze, addze, subfme, addme, add |
| 19, 144 | mfcr, mtcrf |
| 21, 23, 53, 55, 87, 119, 149, 151, 181, 183, 215, 247, 279, 311, 341, 343, 373, 375, 407, 439 | ldx/lwzx/lbzx/… + update forms + stores |
| 24, 26, 27, 58, 536, 539, 792, 794, 824, 826/827 | slw, cntlzw, sld, cntlzd, srw, srd, sraw, srad, srawi, sradi |
| 28, 60, 124, 284, 316, 412, 444, 476 | and, andc, nor, eqv, xor, orc, or, nand |
| 233, 235, 457, 459, 489, 491 | mulld, mullw, divdu, divwu, divd, divw |
| 339, 467 | mfspr / mtspr (LR, CTR, XER) |
| 922, 954, 986 | extsh, extsb, extsw |
| 54, 86, 246, 278, 1014 | cache ops (nop) |

## Still TODO (high value)

- Full XER (CA / OV / SO) modeling on add/sub/carry forms
- Atomics: `lwarx` / `stwcx.` / `ldarx` / `stdcx.`
- FPU (`lfs`/`lfd`/`stfs`/`stfd`/`fadd`/`fmul`/`fmadd`/`fcmpu`/`frsp`/`fctiwz`/…)
- VMX / AltiVec (128-bit vector → SSE/AVX)
- `rldcl` / `rldcr` (variable-shift forms of op 30)
- Trap instructions (`tw`/`td`/`twi`/`tdi`)
- Remaining SPR moves (TB, VRSAVE, etc.)
- Function discovery / per-function emission (instead of fixed chunks)

Everything else is still emitted as `ps3rt_unimplemented()` and counted in `codebase/lift_report.txt`.
