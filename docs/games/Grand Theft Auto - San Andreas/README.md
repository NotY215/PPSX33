# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="180" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas

This folder records the current PPSX33 static PPU lift and ELF-analysis snapshot for Grand Theft Auto: San Andreas.

## Current results

| Lift metric | Result |
| --- | ---: |
| Instruction instances | 1,893,428 |
| Translated instances reported | 1,893,428 |
| Unimplemented instances reported | 0 |
| Chunks | 232 |
| Reported static translation coverage | 100% for this input snapshot |

| ELF / analysis metric | Result |
| --- | ---: |
| PT_LOAD segments | 5 |
| Entry OPD | `0x77b718` |
| Detected symbols | 0 |
| OPD entries | 4,097 |
| PRX/module string hits | 23 |
| Embedded SPU images detected | 0 |

## Reports

- [`lift_report.txt`](lift_report.txt)
- [`analysis_report.txt`](analysis_report.txt)

## Follow-up work

- [ ] Record the exact input build, region, and PPSX33 revision used for these reports.
- [ ] Review approximate operations, fallback paths, and opcode semantics independently of the translated count.
- [ ] Review PRX/module strings and OPD discovery as heuristic findings.
- [ ] Add regression tests for any identified instruction gaps.
- [ ] Continue with the planned GTA V opcode and coverage pass after this snapshot is documented.

## Interpretation

The 100% figure means the lifter reported all 1,893,428 instruction instances in this input as translated. It does not prove that emitted operations preserve PowerPC semantics or that the title boots or is playable. Zero detected symbols does not mean the input has no functions, and zero embedded SPU images means none were detected by this analysis pass.

Do not commit commercial-game executables, decryption keys, or other copyrighted game data.
