# Uncharted 2: Among Thieves PPU Analysis

## Scope

This record documents one completed ELF analysis and static-lifting snapshot for Uncharted 2: Among Thieves. It is a PPU opcode-analysis input, not a runtime compatibility or playability result. SPU execution, RSX/GCM, operating-system services, and complete game support remain separate workstreams.

## Recorded environment and analysis

| Metric | Result |
| --- | --- |
| Local project directory | `E:\\PPSX33\\build\\dist\\UNCHARTED 2` |
| Project folders | `input/`, `codebase/`, `output/` |
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
| Static translation coverage | 100% for this report snapshot |

The ELF was copied into the project's `input/` directory. The tool generated `codebase/lift_report.txt` and `codebase/analysis_report.txt`. These report files remain in the local project and have not been committed to this repository.

## Interpretation and limits

The result means the lifter reported all 3,485,016 instruction instances as translated for this input snapshot. It does not prove semantic correctness. Approximate translations and no-op fallbacks may be counted as translated, so zero reported unimplemented instances is not equivalent to complete PPU behavior.

The analysis reported no symbols and discovered 116 OPDs. These are separate fields; zero symbols must not be interpreted as zero functions. The 391 PRX hits are heuristic findings and are not proof of complete PRX import/export resolution. Detection of seven embedded SPU images does not establish correct SPU execution.

## Developer analysis workflow

1. Review `codebase/lift_report.txt` and `codebase/analysis_report.txt` in the local project.
2. Identify unsupported, approximate, or fallback operations and distinguish them from instructions merely absent in the analyzed input.
3. Inspect the relevant decoder/lifter path in `src/core/ppu_lifter.cpp` and helper behavior in `runtime/ppu_runtime.h`.
4. Verify operand fields, record-bit behavior, condition-register effects, carry/overflow, memory endianness, and exceptions against a trusted PowerPC reference.
5. Add focused synthetic regression tests when practical.
6. Run synthetic tests and relevant existing baselines after changes.
7. Re-run the lift and update this record only when new output supports the change.

## Remaining work

- Review the generated reports for approximate or fallback instruction handling.
- Establish semantic test coverage for the instruction families exercised by this snapshot.
- Archive suitable text reports in the repository only after reviewing them for sensitive paths and proprietary content.
- Re-run the GOW3 baseline after lifter changes and compare both counts and behavior.
- Keep runtime compatibility, SPU execution, RSX rendering, and system-library support tracked separately.

## Artifact policy

Do not commit commercial-game ELFs, EBOOT files, keys, copyrighted assets, or generated game executables. Static translation counts must not be presented as semantic correctness or game compatibility.

## Related documentation

- [Directory overview](README.md)
- [Game coverage index](../README.md)
- [PPU coverage methodology](../../PPU_COVERAGE.md)
- [Developer guide](../../DEVELOPER_GUIDE.md)
