# Uncharted 2: Among Thieves

**Status:** Planned static-lifting and PPU opcode-analysis target  
**Measurements:** Not yet recorded  
**Current scope:** Analyze reports and improve PPU opcode coverage; runtime compatibility is not claimed.

## Purpose

The title is an additional input for discovering PPU instructions and instruction forms that are missing, approximate, or not exercised by the GOW3 snapshot. Work from actual lift reports and isolate behavior in regression tests where practical.

## Workflow

1. Use an authorized decrypted PS3 ELF.
2. Run static lifting and preserve the generated text reports.
3. Review missing, approximate, or fallback instruction handling.
4. Prioritize opcode changes using report evidence and semantic risk.
5. Add focused tests and re-run existing synthetic tests.
6. Record real before-and-after counts in the [analysis plan](UNCHARTED2.md).

Do not publish instruction counts before generating a real report. Do not commit the game ELF, EBOOT, keys, assets, or generated commercial-game binaries.

## Related documentation

- [Detailed PPU analysis plan](UNCHARTED2.md)
- [Game coverage index](../README.md)
- [PPU coverage methodology](../../PPU_COVERAGE.md)
- [Roadmap](../../../ROADMAP.md)
