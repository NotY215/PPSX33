# PPU instruction coverage (`src/core/ppu_lifter.cpp: lift_one`)

Primary opcode to status. "Rc" means the record bit also sets CR0 / CR1.

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
| 19 | bclr, bcctr, isync, sync, eieio, mcrf, crand/cror/crxor/... | CR logicals + barriers as nop |
| 21 | rlwinm (slwi, srwi, clrlwi, ...) | Rc supported |
| 24-29 | ori, oris, xori, xoris, andi., andis. | |
| 30 | rldicl, rldicr, rldic, rldimi | 64-bit rotates (Cell-critical) |
| 31 | integer X-form (see previous) + FPU indexed ld/st | |
| 32-35 | lwz, lwzu, lbz, lbzu | |
| 36-39 | stw, stwu, stb, stbu | |
| 40-43 | lhz, lhzu, lha, lhau | |
| 44-45 | sth, sthu | |
| 46 / 47 | lmw / stmw | |
| 48-55 | lfs, lfsu, lfd, lfdu, stfs, stfsu, stfd, stfdu | FPU memory |
| 58 / 62 | ld, ldu, lwa / std, stdu | DS-form |
| 59 | single-precision arithmetic subset | fadds, fsubs, fmuls, fdivs, fmadds, ... |
| 63 | double-precision arithmetic subset | fadd, fsub, fmul, fdiv, fmadd, fmr, fcmpu, frsp, fctiwz, fabs, fneg, ... |

## Still TODO (high value)

- Full XER (CA / OV / SO) modeling on add/sub/carry forms
- Atomics: `lwarx` / `stwcx.` / `ldarx` / `stdcx.`
- Remaining FPU (fres, frsqrte, fsel, mffs, mtfsf, more rounding modes)
- VMX / AltiVec (128-bit vector to SSE/AVX)
- `rldcl` / `rldcr` (variable-shift forms of op 30)
- Trap instructions (`tw` / `td` / `twi` / `tdi`)
- Remaining SPR moves (TB, VRSAVE, etc.)
- Function discovery / per-function emission (instead of fixed chunks)

Everything else is still emitted as `ps3rt_unimplemented()` and counted in `codebase/lift_report.txt`.
