# <img src="https://thumb.wikimedia.org/wikipedia/it/thumb/7/76/Grand_Theft_Auto_V_logo.svg/330px-Grand_Theft_Auto_V_logo.svg.png?utm_source=it.wikipedia.org&utm_campaign=index&utm_content=thumbnail&_=20240305183247" width="180" alt="Grand Theft Auto V logo" /> Grand Theft Auto V

## Analysis status

**Static lift and ELF analysis recorded.** The metrics below describe the uploaded report snapshot; exact input build and provenance still need to be documented.

## Planned lift metrics

| Metric | Result |
| --- | --- |
| Input build / region | Not recorded |
| Instruction instances | 6,934,214 |
| Translated instances reported | 6,934,214 |
| Unimplemented instances reported | 0 |
| Chunks | 847 |
| Reported static translation coverage | 100% for this snapshot |
| Lift report | [lift_report.txt](lift_report.txt) |
| Analysis report | [analysis_report.txt](analysis_report.txt) |

## Planned ELF and opcode findings

| Metric | Result |
| --- | --- |
| PT_LOAD segments | 5 |
| Entry OPD | `0x1a90b80` |
| Detected symbols | 0 |
| OPD entries | 4,097 |
| PRX/module string hits | 56 |
| Embedded SPU images | 8 |
| Opcode families requiring review | Pending semantic review |

## Evaluation checklist

- [x] Run ELF analysis and preserve `analysis_report.txt`.
- [x] Run static PPU lifting and preserve `lift_report.txt`.
- [ ] Record the exact GTA V input build and its provenance.
- [ ] Compare opcode-family findings with existing title snapshots.
- [ ] Inspect approximate and fallback operations rather than relying only on translated counts.
- [ ] Add regression tests for newly identified instruction patterns.

The reports record 100% static translation for this input snapshot. This does not prove correct instruction semantics, runtime compatibility, or playability; approximate operations and fallback translations may still be counted as translated.
