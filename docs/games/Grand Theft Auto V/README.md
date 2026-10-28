# <img src="https://thumb.wikimedia.org/wikipedia/it/thumb/7/76/Grand_Theft_Auto_V_logo.svg/330px-Grand_Theft_Auto_V_logo.svg.png?utm_source=it.wikipedia.org&utm_campaign=index&utm_content=thumbnail&_=20240305183247" width="180" alt="Grand Theft Auto V logo" /> Grand Theft Auto V

This folder records the PPSX33 static-lift and ELF-analysis snapshot for Grand Theft Auto V.

## Planned work

- [x] Archive the GTA V `lift_report.txt` and `analysis_report.txt` snapshot.
- [ ] Record the exact GTA V executable/input build and provenance.
- [ ] Review opcode families, approximate operations, fallback paths, and semantic gaps beyond headline coverage.
- [ ] Add focused synthetic regression tests for newly discovered instruction patterns.

## Current status

**Static analysis recorded; semantic and runtime validation remain pending.** The archived report records 6,934,214 instruction instances, 6,934,214 translated, 0 reported unimplemented, and 847 chunks. ELF analysis records 5 PT_LOAD segments, entry OPD `0x1a90b80`, 0 detected symbols, 4,097 OPD entries, 56 PRX/module string hits, and 8 embedded SPU images. These are report metrics for this input snapshot, not proof of correct instruction semantics or playability.

## Files

- [Grand Theft Auto V.md](Grand%20Theft%20Auto%20V.md): title-specific tracking record.
- - [lift_report.txt](lift_report.txt): static-lift metrics.
- [analysis_report.txt](analysis_report.txt): ELF analysis metrics.

## Important notes

Keep future metrics tied to the exact input build and PPSX33 revision. Translation coverage is not proof of semantic correctness, runtime compatibility, or playability. Do not commit commercial-game executables or other copyrighted game data.
