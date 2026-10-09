# PPU coverage status (PPSX33)

GoW3 snapshot:

- Instructions: 1,285,560
- Translated: 1,103,166 (~86%)
- Unimplemented: 182,394
- MSVC build: OK (game.exe + ps3rt.dll)

## Implemented (major groups)

- Integer ALU, logical, shifts, rotates
- Loads/stores (common + update + indexed + lmw/stmw)
- Branches, CR logicals, SPR (LR/CTR/XER)
- FPU memory + arithmetic subset
- Atomics stubs, traps as nop (in progress on lifter)

## Top remaining (GoW3)

| Key | Count | Notes |
|-----|------:|-------|
| op=4 | 76k | VMX / AltiVec |
| op=31 xo=103 | 19k | |
| op=9 | 13k | |
| op=31 xo=231 | 13k | |
| op=6/1/3/2/5 | ~39k | rare / data-as-code |

## Runtime

When PC leaves recompiled range the runtime prints pc/lr/ctr/r1/r2.
Odd PCs (like 0x102300052d6c8) mean a bad function pointer or LR/CTR.
