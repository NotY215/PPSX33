# PPSX33 Game Coverage Records

These records describe static lifting snapshots collected while evaluating PPSX33. Counts refer to instruction instances in a particular lift report. They do not establish semantic correctness, successful boot, correct rendering, or playability.

## Recorded games

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Reported static coverage |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III (GOW3) | [GOW3.md](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% |

The GOW3 result means that this snapshot reported no instruction instances as unimplemented. Some emitted semantics may still be approximate or no-op fallbacks. See [PPU coverage](../PPU_COVERAGE.md) before interpreting the percentage as broader ISA support.
