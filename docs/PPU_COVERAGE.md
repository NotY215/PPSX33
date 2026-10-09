# PPU instruction coverage

Primary opcode to status.

## Implemented (high level)

- Integer arithmetic, logical, shifts, rotates (rlwinm, rldicl/rldicr/rldic/rldimi, rldcl/rldcr, rlwnm)
- Loads/stores including update and indexed forms, lmw/stmw
- Branches, CR logicals, mfcr/mtcrf, mfspr/mtspr (LR/CTR/XER)
- FPU memory: lfs/lfsu/lfd/lfdu/stfs/stfsu/stfd/stfdu + indexed forms
- FPU arithmetic subset (op 59 / 63): fadd/fsub/fmul/fdiv/fmadd/fmsub/fnmsub, fmr, fcmpu, frsp, fctiwz, fabs, fneg, ...

## Top remaining from GoW3 lift_report

| Key | Approx count | Meaning |
|-----|--------------|---------|
| op=4 | 76k | VMX / AltiVec (vector) |
| op=0 | 39k | illegal / padding in code segments |
| op=31 xo=103 | 19k | (decode next) |
| op=9 | 13k | (decode next) |
| op=6 | 10k | (decode next) |
| remaining FPU / exotic XO | ... | fres, frsqrte, fsel, mffs, mtfsf, atomics |

## Still TODO

- Full XER CA/OV/SO
- Atomics (lwarx/stwcx./ldarx/stdcx.)
- Full FPU (fres, frsqrte, fsel, mffs, mtfsf, all rounding)
- VMX / AltiVec -> SSE/AVX
- Trap instructions
- Function-level emission instead of fixed chunks
