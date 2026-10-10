# <img src="https://commons.wikimedia.org/wiki/Special:FilePath/God_of_War_Logo.png" width="140" alt="God of War logo" /> God of War III (GOW3)

This folder stores the documented static-lift and analysis snapshots for God of War III, PPSX33's primary runtime target.

## Current snapshot

| Metric | Value |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated instances reported | 1,285,560 |
| Unimplemented instances reported | 0 |
| Chunks | 157 |
| PT_LOAD segments | 5 |
| Entry OPD | `0x50ddc0` |
| Detected symbols | 0 |
| OPD entries (heuristic) | 3 |
| PRX/module string hits | 47 |
| Embedded SPU images | 8 |
| Reported static translation coverage | 100% for this snapshot |

## Files

- [GOW3.md](GOW3.md): lift statistics, analysis notes, and runtime observations.
- [GOW3_lift_report.txt](GOW3_lift_report.txt): archived lift report.
- [analysis_report.txt](analysis_report.txt): archived ELF and module analysis.

## Status and limitations

GOW3 is the primary runtime-stability target. The reported 100% static translation coverage only means the instructions in this snapshot were classified as translated. It does not establish correct PPU semantics, complete import resolution, or successful gameplay. Approximate operations and no-op fallbacks may still be counted as translated.

Do not commit game executables, decryption keys, or other copyrighted game data.
