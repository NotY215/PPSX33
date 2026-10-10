# PPSX33 Game Coverage Records

This directory stores reproducible static-lifting snapshots and title-specific analysis records. Instruction counts describe the exact input and report version recorded. They do not establish semantic correctness, boot, rendering, or playability.

## Recorded coverage

| Game | Record | Instructions | Translated | Unimplemented | Chunks | Static coverage |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| God of War III (GOW3) | [GOW3.md](GOW3/GOW3.md) | 1,285,560 | 1,285,560 | 0 | 157 | 100% for this snapshot |
| Uncharted 2: Among Thieves | [Uncharted2/README.md](Uncharted2/README.md) | 3,485,016 | 3,485,016 | 0 | 426 | 100% for this snapshot |

Both results are static translation metrics only. Approximate operations and no-op fallbacks may still be counted as translated. Uncharted 2's analysis also reported 5 ELF load segments, entry OPD `0xdd8618`, 0 symbols, 116 OPDs, 391 PRX heuristic hits, and 7 embedded SPU images. Its full text reports remain in the local project at `E:\\PPSX33\\build\\dist\\UNCHARTED 2\\codebase\\` and are not currently archived in this repository.

## Opcode analysis status

Uncharted 2 has completed its initial static lift. The next analysis step is to inspect its generated reports for approximate operations, no-op fallbacks, and semantic test gaps. The result alone does not prove that it exposes additional opcodes or that translated instructions behave correctly.

## Adding or updating a game record

1. Use input files legally obtained and authorized for analysis.
2. Preserve the tool version or commit, input identification, command/workflow, and date for each report snapshot when available.
3. Copy only generated text reports suitable for public release. Inspect them for sensitive paths or extracted proprietary content before committing.
4. Record instruction instances, translated instances, unimplemented instances, and chunk counts exactly as emitted by the tool.
5. Separate decode and translation counts from semantic validation, runtime results, and game compatibility.
6. Update the game record and this index together. Do not invent missing metrics.

See [PPU coverage methodology](../PPU_COVERAGE.md) and the [developer guide](../DEVELOPER_GUIDE.md).
