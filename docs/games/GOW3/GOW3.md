# PPSX33 Game Record: God of War III (GOW3)

## Lift statistics

| Metric | Count |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated / implemented in report | 1,285,560 |
| Unimplemented in report | 0 |
| Chunks | 157 |
| Reported static translation coverage | 100% |

The accompanying [lift report](GOW3_lift_report.txt) contains the raw totals.

## Interpretation

The current snapshot classifies all 1,285,560 instruction instances as translated. This is a static-lifting metric for this input and report only. It does not mean that every emitted instruction has exact PowerPC semantics, nor does it establish successful boot, correct rendering, or playability. The lifter contains approximate handling and no-op fallbacks for some less common operations.

Runtime correctness, system calls, PRX imports, SPU workloads, RSX graphics, and guest control flow must be validated independently. Use this record alongside [PPU coverage](../../PPU_COVERAGE.md); do not generalize the result to every game or the complete PowerPC ISA.
