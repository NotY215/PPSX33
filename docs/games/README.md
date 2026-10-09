# PPSX33 Game Coverage Records

This directory stores reproducible static-lifting snapshots and title-specific analysis records. Instruction counts describe the exact input and report version recorded. They do not establish semantic correctness, boot, rendering, or playability.

## Recorded coverage

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Static coverage |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III (GOW3) | [GOW3.md](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% for this snapshot |

The GOW3 result is a static translation metric only. Approximate operations and no-op fallbacks may still be counted as translated.

## Opcode-expansion target

| Game | Record | Status | Purpose |
| --- | --- | --- | --- |
| Uncharted 2: Among Thieves | [Uncharted2/README.md](Uncharted2/README.md) | Planned analysis target; counts not measured | Find missing or approximate PPU opcode behavior and add regression tests |

## Adding or updating a game record

1. Use input files legally obtained and authorized for analysis.
2. Preserve the tool version or commit, input identification, command/workflow, and date for each report snapshot. Do not publish copyrighted game data.
3. Copy only the generated text reports needed to reproduce the analysis. Inspect them for sensitive paths or extracted proprietary content before committing.
4. Record instruction instances, translated instances, unimplemented instances, and chunk counts exactly as emitted by the tool.
5. Separate decode and translation counts from semantic validation, runtime results, and game compatibility.
6. Update the game record and this index together. Do not invent missing metrics.

See [PPU coverage methodology](../PPU_COVERAGE.md) and the [developer guide](../DEVELOPER_GUIDE.md).
