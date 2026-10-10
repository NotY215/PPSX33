# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/Grand_Theft_Auto_San_Andreas_logo.svg" width="180" alt="Grand Theft Auto: San Andreas logo" /> Grand Theft Auto: San Andreas

This folder is reserved for future PPSX33 analysis of Grand Theft Auto: San Andreas.

## Planned work

- [ ] Identify and document the supported, legally obtained input ELF format.
- [ ] Run static ELF analysis and archive the resulting `analysis_report.txt`.
- [ ] Run the PPU lifter and archive the resulting `lift_report.txt`.
- [ ] Record instruction totals, translated and unimplemented counts, chunk totals, ELF segments, entry OPD, OPD entries, PRX/module string hits, and embedded SPU image findings where available.
- [ ] Review approximate operations, fallback paths, and import resolution separately from raw translation coverage.
- [ ] Add regression tests for newly discovered PPU instruction patterns.

## Current status

**Planned; not yet analyzed.** No lift or analysis metrics are recorded for this title yet. Do not interpret missing reports as zero instructions or 100% coverage.

## Files

- [Grand Theft Auto - San Andreas.md](Grand%20Theft%20Auto%20-%20San%20Andreas.md): title-specific tracking record.
- `lift_report.txt` and `analysis_report.txt`: to be added after a real analysis run.

## Important notes

Static translation totals alone do not prove semantic correctness or playability. Keep any future reports tied to the exact input build and tool version. Do not commit commercial-game executables, decryption keys, or other copyrighted game data.
