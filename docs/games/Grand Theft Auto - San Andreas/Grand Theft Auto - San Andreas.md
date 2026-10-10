# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="180" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas

## Analysis status

**Planned; not yet analyzed.**

This record tracks a future PPSX33 static-lift and ELF-analysis pass. No measurements have been supplied or generated yet.

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

## Planned ELF and analysis metrics

| Metric | Result |
| --- | --- |
| PT_LOAD segments | Pending |
| Entry OPD | Pending |
| Detected symbols | Pending |
| OPD entries | Pending |
| PRX/module string hits | Pending |
| Embedded SPU images | Pending |

## Evaluation checklist

- [ ] Record the exact input build and source of the legally obtained ELF.
- [ ] Run ELF analysis and preserve `analysis_report.txt`.
- [ ] Run static PPU lifting and preserve `lift_report.txt`.
- [ ] Inspect approximate and fallback operations instead of relying only on the translated count.
- [ ] Review imports and runtime dependencies separately.
- [ ] Add regression tests for any newly identified instruction families.

Do not fill in metrics until they are produced by an actual analysis run. A reported 100% translation rate, if reached, would not alone prove correct semantics or playability.
