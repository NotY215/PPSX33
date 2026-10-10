# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_V_logo.svg" width="180" alt="Grand Theft Auto V logo" /> Grand Theft Auto V

## Analysis status

**Planned; not yet analyzed.** GTA V coverage work is scheduled after the Grand Theft Auto: San Andreas coverage pass.

## Planned lift metrics

| Metric | Result |
| --- | --- |
| Input build / region | Pending |
| Instruction instances | Pending |
| Translated instances reported | Pending |
| Unimplemented instances reported | Pending |
| Chunks | Pending |
| Reported static translation coverage | Pending |
| Lift report | Not generated |
| Analysis report | Not generated |

## Planned ELF and opcode findings

| Metric | Result |
| --- | --- |
| PT_LOAD segments | Pending |
| Entry OPD | Pending |
| Detected symbols | Pending |
| OPD entries | Pending |
| PRX/module string hits | Pending |
| Embedded SPU images | Pending |
| Opcode families requiring review | Pending |

## Evaluation checklist

- [ ] Finish the planned Grand Theft Auto: San Andreas coverage pass first.
- [ ] Record the exact GTA V input build and its provenance.
- [ ] Run ELF analysis and preserve `analysis_report.txt`.
- [ ] Run static PPU lifting and preserve `lift_report.txt`.
- [ ] Compare opcode-family findings with existing title snapshots.
- [ ] Inspect approximate and fallback operations rather than relying only on translated counts.
- [ ] Add regression tests for newly identified instruction patterns.

Do not fill in metrics until an actual analysis run produces them. A reported 100% translation rate would not alone prove correct instruction semantics or playability.
