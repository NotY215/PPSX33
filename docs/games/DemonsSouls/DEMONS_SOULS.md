# Demon's Souls — static lift track

**Role:** Secondary static-coverage target (with Uncharted 2). Primary runtime remains GOW3.

**Build/run:** Do not treat native `game.exe` for Demon's Souls as a goal until GOW3 boots further.

## Latest lift snapshot

*Not yet recorded.* Run decrypt + decompile in PPSX33 and paste metrics into the table below and into [README.md](README.md).

| Metric | Value |
| --- | ---: |
| Segments | — |
| Entry OPD | — |
| Symbols | — |
| OPD entries | — |
| PRX string hits | — |
| Embedded SPU images | — |
| Instruction instances | — |
| Translated | — |
| Unimplemented | — |
| Chunks | — |
| Reported static coverage | — |

## Interpretation

Same caveats as Uncharted 2 / GOW3: zero unimplemented means every instruction word was emitted as some C++ (including approximate/nop paths). It does not prove semantic correctness or playability.

## Workflow

1. GUI: choose **rpcs3.exe** (OPTIONS or on first Decrypt).
2. **Decrypt EBOOT** → load ELF into a project named e.g. `DemonsSouls`.
3. **Decompile** only; skip Build for this title.
4. Diff approximate ops / SPU images against GOW3 and Uncharted 2.
5. Harden lifter/runtime from findings; re-lift GOW3 after changes.

See [ROADMAP](../../../ROADMAP.md).
