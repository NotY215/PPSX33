# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="180" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas

## Analysis status

**Lift and ELF analysis reports recorded.** The current snapshot reports 1,893,428 instruction instances, all reported translated, 0 reported unimplemented, and 232 chunks.

## Lift metrics

| Metric | Result |
| --- | --- |
| Input build / region | Not recorded in the report |
| Instruction instances | 1,893,428 |
| Translated instances reported | 1,893,428 |
| Unimplemented instances reported | 0 |
| Chunks | 232 |
| Reported static translation coverage | 100% for this snapshot |
| Lift report | [lift_report.txt](lift_report.txt) |

## ELF and analysis metrics

| Metric | Result |
| --- | --- |
| PT_LOAD segments | 5 |
| Entry OPD | `0x77b718` |
| Detected symbols | 0 |
| OPD entries | 4,097 |
| PRX/module string hits | 23 |
| Embedded SPU images detected | 0 |
| Analysis report | [analysis_report.txt](analysis_report.txt) |

## Evaluation checklist

- [ ] Record the exact input build/region and PPSX33 revision used for this snapshot.
- [ ] Inspect approximate and fallback translations instead of relying only on translated counts.
- [ ] Review PRX/module string and OPD findings as heuristic analysis results.
- [ ] Add regression tests for newly identified instruction patterns.
- [ ] Begin the planned GTA V opcode and coverage pass after documenting this snapshot.

## Interpretation

A reported 100% translation rate means every instruction instance in this particular input was classified as translated. It does not establish semantic correctness, successful boot, runtime compatibility, or playability. Zero detected symbols does not mean no functions exist, and zero detected SPU images does not prove the title has no SPU-related behavior.
