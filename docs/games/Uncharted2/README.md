# Uncharted 2: Among Thieves

**Status:** ELF analysis and static lifting completed for the recorded input snapshot  
**Runtime compatibility:** Not established

## Recorded lift results

| Metric | Result |
| --- | ---: |
| ELF load segments | 5 |
| Entry | OPD at `0xdd8618` |
| Symbols detected | 0 |
| OPDs detected | 116 |
| PRX heuristic hits | 391 |
| Embedded SPU images | 7 |
| Instruction instances | 3,485,016 |
| Translated instances reported | 3,485,016 |
| Unimplemented instances reported | 0 |
| Chunks | 426 |
| Static translation coverage | 100% for this snapshot |

The analysis created the project at `E:\\PPSX33\\build\\dist\\UNCHARTED 2` with `input/`, `codebase/`, and `output/` directories. The ELF was copied into `input/`. The generated reports are `codebase/lift_report.txt` and `codebase/analysis_report.txt` within that local project.

These report files have not been included in this repository. The values above are the recorded console output and must not be treated as a substitute for archiving and reviewing the full reports.

## Interpretation

The lifter reported every instruction instance in this input as translated. This does not establish that each translated instruction has correct PowerPC semantics. Approximate operations and no-op fallbacks may still be counted as translated. Zero detected symbols also does not mean the ELF has no functions; OPD and function discovery can use other analysis heuristics.

The seven detected SPU images are analysis findings, not evidence that SPU programs execute correctly. PRX hits are heuristic detections, not confirmed import/export resolution.

## Follow-up analysis

1. Inspect `codebase/lift_report.txt` and `codebase/analysis_report.txt` in the generated project.
2. Identify approximate operations, no-op fallbacks, and instruction families that lack semantic regression tests.
3. Compare instruction handling with the PowerPC specification or a trusted reference.
4. Add focused synthetic regression tests before changing the lifter.
5. Re-run this target and the GOW3 baseline after opcode changes.
6. Record any changes to counts only from new tool output.

## Artifact policy

Do not commit commercial-game ELFs, EBOOT files, keys, copyrighted assets, or generated game executables. Only publish text reports that are appropriate for public release and have been checked for sensitive paths or proprietary content. Static translation counts must not be presented as semantic correctness or game compatibility.

## Related documentation

- [Directory overview](README.md)
- [Game coverage index](../README.md)
- [PPU coverage methodology](../../PPU_COVERAGE.md)
- [Developer guide](../../DEVELOPER_GUIDE.md)
