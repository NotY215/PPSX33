# <img src="https://www.pngkey.com/png/detail/983-9839044_uncharted-2-among-thieves.png" width="68" alt="Uncharted 2 logo artwork" /> Uncharted 2: Among Thieves — PPU opcode expansion

**Role:** Secondary target for static PPU coverage. Primary runtime target remains GOW3.

**Build/run:** Do not build or launch native `game.exe` for Uncharted 2 until GOW3 runtime is further along.

## Latest lift snapshot

| Metric | Value |
| --- | ---: |
| Segments | 5 |
| Entry OPD | 0xdd8618 |
| Symbols | 0 |
| OPD entries | 116 |
| PRX string hits | 391 |
| Embedded SPU images | 7 |
| Instruction instances | 3,485,016 |
| Translated | 3,485,016 |
| Unimplemented | 0 |
| Chunks | 426 |
| Reported static coverage | 100% |

Source: user-supplied decompile log (project `UNCHARTED 2`). Archive a full `lift_report.txt` under this folder when convenient.

## Interpretation

Same caveats as GOW3: zero unimplemented means every instruction word was emitted as some C++ (including approximate/nop paths). It does not prove semantic correctness or playability.

Use this title to stress-test long-tail encodings and the 7 extracted SPU images against the SPU interpreter.

## Workflow

1. Decrypt via GUI **Decrypt EBOOT (RPCS3)** (`rpcs3 --decrypt`) or load an existing `.elf`.
2. Decompile only; skip Build for this title.
3. Diff approximate ops / runtime behavior against GOW3.
4. Harden lifter/runtime; re-lift both titles.

See [ROADMAP](../../../ROADMAP.md).
