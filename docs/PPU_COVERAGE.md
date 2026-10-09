# PPSX33 PPU Instruction Coverage

## God of War III lift snapshot

| Metric | Result |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated | 1,103,166 (approximately 85.81%) |
| Unimplemented | 182,394 |
| Chunks | 157 |
| MSVC artifact build | Successful: `game.exe` and `ps3rt.dll` |
| Runtime execution | Incomplete: execution starts, then the guest PC leaves the recompiled range |

## Implemented instruction groups

- Integer arithmetic, logical operations, shifts, and rotates
- Loads and stores, including common update and indexed forms
- Branches, condition-register logical operations, and LR/CTR/XER access
- A subset of floating-point memory and arithmetic operations
- Partial atomic handling and trap behavior

## Highest-count remaining groups in this snapshot

| Instruction group | Approximate count | Family |
| --- | ---: | --- |
| Primary opcode 4 | 76,000 | VMX / AltiVec |
| Primary opcode 31, XO 103 | 19,000 | Integer / system instruction family |
| Primary opcode 9 | 13,000 | Extended arithmetic family |
| Primary opcode 31, XO 231 | 13,000 | Integer / system instruction family |
| Primary opcodes 6, 1, 3, 2, and 5 | 39,000 | Remaining groups, including possible data interpreted as code |

Counts are from the recorded lift snapshot and should be regenerated after changes to the lifter.

## Runtime diagnostics

When the guest PC leaves the recompiled range, the runtime reports `pc`, `lr`, `ctr`, `r1`, and `r2`. These values are diagnostic evidence, not a definitive root-cause diagnosis. Runtime investigation should check branch target calculations, LR/CTR state, indirect branches, missing lifted chunks, and function-descriptor/OPD handling.

MSVC currently emits `game.exe` and `ps3rt.dll`, but correct guest execution and game compatibility are not established. PPU coverage, system-call and PRX support, SPU execution, and RSX graphics are separate requirements.
