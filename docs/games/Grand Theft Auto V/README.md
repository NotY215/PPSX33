# <img src="https://thumb.wikimedia.org/wikipedia/it/thumb/7/76/Grand_Theft_Auto_V_logo.svg/330px-Grand_Theft_Auto_V_logo.svg.png?utm_source=it.wikipedia.org&utm_campaign=index&utm_content=thumbnail&_=20240305183247" width="180" alt="Grand Theft Auto V logo" /> Grand Theft Auto V

This folder tracks a future PPSX33 opcode and static-translation coverage study for Grand Theft Auto V, planned after the Grand Theft Auto: San Andreas coverage pass.

## Planned work

- [ ] Complete and document the Grand Theft Auto: San Andreas coverage pass first.
- [ ] Identify the exact GTA V executable/input build and record its provenance.
- [ ] Run static ELF analysis and archive the resulting `analysis_report.txt`.
- [ ] Run the PPU lifter and archive the resulting `lift_report.txt`.
- [ ] Record instruction totals, translated and unimplemented counts, chunks, PT_LOAD segments, entry OPD, detected symbols, OPD entries, PRX/module string hits, and embedded SPU findings where applicable.
- [ ] Review opcode families, approximate operations, fallback paths, and semantic gaps beyond headline coverage.
- [ ] Add focused synthetic regression tests for newly discovered instruction patterns.

## Current status

**Planned; not yet analyzed.** No GTA V lift or analysis metrics are recorded. Missing reports do not mean zero instructions or complete coverage.

## Files

- [Grand Theft Auto V.md](Grand%20Theft%20Auto%20V.md): title-specific tracking record.
- `lift_report.txt` and `analysis_report.txt`: to be added after actual analysis.

## Important notes

Keep future metrics tied to the exact input build and PPSX33 revision. Translation coverage is not proof of semantic correctness, runtime compatibility, or playability. Do not commit commercial-game executables or other copyrighted game data.
