# PPSX33 PPU Instruction Coverage

## God of War III lift snapshot

| Metric | Result |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated / implemented | 1,285,560 (100%) |
| Unimplemented | 0 |
| Chunks | 157 |
| Static translation coverage | 100% |
| MSVC artifact build | Successful: `game.exe` and `ps3rt.dll` |
| Runtime execution | Incomplete: execution starts, then the guest PC leaves the recompiled range |

The current GOW3 lift report records no unimplemented instructions for this snapshot. This is a per-snapshot result, not proof that PPSX33 supports every PowerPC instruction or every instruction used by other games.

## Implemented instruction groups

The lifter's implementation is tracked in `src/core/ppu_lifter.cpp`. The GOW3 report measures the instructions encountered in this specific lift; use dedicated tests and additional games to discover gaps outside this sample.

## Runtime diagnostics

When the guest PC leaves the recompiled range, the runtime reports `pc`, `lr`, `ctr`, `r1`, and `r2`. These values are diagnostic evidence, not a definitive root-cause diagnosis. Runtime investigation should check branch target calculations, LR/CTR state, indirect branches, missing lifted chunks, and function-descriptor/OPD handling.

MSVC currently emits `game.exe` and `ps3rt.dll`, but correct guest execution and game compatibility are not established. PPU instruction coverage, system-call and PRX support, SPU execution, and RSX graphics are separate requirements.
