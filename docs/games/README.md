# PPSX33 Game Coverage Records

Static lifting snapshots only. Counts are instruction instances in a given report. They do not prove semantic correctness, boot, rendering, or playability.

**Priority:** God of War III is the primary runtime target. Uncharted 2 is used for opcode expansion (lift only; no build/run yet).

## Recorded games

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Static coverage |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III (GOW3) | [GOW3.md](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% |

## Secondary / planned

| Game | Record | Status | Purpose |
| --- | --- | --- | --- |
| Uncharted 2: Among Thieves | [Uncharted2/](Uncharted2/) | Opcode expansion; lift only | Missing/weak PPU opcodes; no native exe yet |

See [PPU coverage](../PPU_COVERAGE.md) and [ROADMAP](../../ROADMAP.md).
