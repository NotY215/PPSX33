# Demon's Souls — static lift track

**Role:** Secondary static-coverage target (with Uncharted 2). Primary runtime remains GOW3.

**Build/run:** Do not treat native `game.exe` for Demon's Souls as a goal until GOW3 boots further.

## Latest lift snapshot

Recorded from [`DemonsSouls_lift_report.txt`](DemonsSouls_lift_report.txt). This is a static-lift snapshot only; it does not establish semantic correctness or playability.

| Metric | Value |
| --- | ---: |
| Segments | Not included in this lift report |
| Entry OPD | Not included in this lift report |
| Symbols | Not included in this lift report |
| OPD entries | Not included in this lift report |
| PRX string hits | Not included in this lift report |
| Embedded SPU images | Not included in this lift report |
| Instruction instances | 6,343,506 |
| Translated | 6,343,506 |
| Unimplemented | 0 |
| Chunks | 775 |
| Reported static coverage | 100% for this snapshot |

## Interpretation

Same caveats as Uncharted 2 / GOW3: zero unimplemented means every instruction word was emitted as some C++ (including approximate/nop paths). It does not prove semantic correctness or playability.

## Workflow

1. GUI: choose **rpcs3.exe** (OPTIONS or on first Decrypt).
2. **Decrypt EBOOT** → load ELF into a project named e.g. `DemonsSouls`.
3. **Decompile** only; skip Build for this title.
4. Diff approximate ops / SPU images against GOW3 and Uncharted 2.
5. Harden lifter/runtime from findings; re-lift GOW3 after changes.

The archived report lists no missing opcodes, but this does not prove complete semantic support. See [ROADMAP](../../../ROADMAP.md).
