# PPSX33 Game Coverage Records

These records describe static lifting snapshots collected while evaluating PPSX33. Counts refer to instruction instances in a particular lift report. They do not establish semantic correctness, successful boot, correct rendering, or playability.

## Recorded games

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Reported static coverage |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III (GOW3) | [GOW3.md](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% |

The GOW3 result means that this snapshot reported no instruction instances as unimplemented. Some emitted semantics may still be approximate or no-op fallbacks. See [PPU coverage](../PPU_COVERAGE.md) before interpreting the percentage as broader ISA support.

## Planned targets

| Game | Record | Status | Purpose |
| --- | --- | --- | --- |
| Uncharted 2: Among Thieves | [Uncharted2/README.md](Uncharted2/README.md) | Planned, not started | Discover missing or incomplete PPU opcode implementations and expand regression coverage |

No Uncharted 2 instruction counts or coverage claims have been measured yet. See the [analysis plan](Uncharted2/UNCHARTED2.md) for the intended workflow.
