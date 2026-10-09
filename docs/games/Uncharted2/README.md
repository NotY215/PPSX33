# Uncharted 2: Among Thieves

**Status:** Planned future PPU coverage target  
**Current implementation status:** Not started

PPSX33 plans to use the standard edition of *Uncharted 2: Among Thieves* as a future analysis target. Its PPU instruction usage will help identify opcode families and instruction forms that need implementation or better semantic coverage.

## Goals

- Analyze a legally obtained, decrypted PS3 ELF when work on this target begins.
- Record the PPU instructions encountered during static lifting.
- Identify unsupported, approximate, or no-op instruction translations.
- Implement and test missing PPU opcode semantics where needed.
- Add focused regression tests so coverage improvements remain verifiable.

## Reporting rules

No instruction counts, coverage percentage, or compatibility result is claimed for Uncharted 2 at this stage. Add measured results only after a real lift report has been generated and reviewed.

Static translation coverage is not the same as correct instruction semantics, successful runtime execution, rendering, or full-game compatibility. Track those outcomes separately.

## Related records

- [Game coverage index](../README.md)
- [PPU coverage notes](../../PPU_COVERAGE.md)
- [PPSX33 roadmap](../../../ROADMAP.md)
