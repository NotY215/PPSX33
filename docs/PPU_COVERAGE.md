# PPSX33 PPU Instruction Coverage

## Recorded static-lift snapshots

| Game | Instruction instances | Translated instances reported | Unimplemented instances reported | Chunks | Static translation coverage |
| --- | ---: | ---: | ---: | ---: | --- |
| God of War III (GOW3) | 1,285,560 | 1,285,560 | 0 | 157 | 100% for this snapshot |
| Minecraft (PS3) | 3,034,530 | 3,034,530 | 0 | 371 | 100% for this snapshot |
| Uncharted 2: Among Thieves | 3,485,016 | 3,485,016 | 0 | 426 | 100% for this snapshot |
| Demon's Souls | 6,343,506 | 6,343,506 | 0 | 775 | 100% for this snapshot |
| Grand Theft Auto: San Andreas | 1,893,428 | 1,893,428 | 0 | 232 | 100% for this snapshot |
| Grand Theft Auto V | 6,934,214 | 6,934,214 | 0 | 847 | 100% for this snapshot |

## ELF and analysis metadata

| Game | PT_LOAD segments | Entry OPD | Symbols | OPD entries | PRX/module string hits | Embedded SPU images |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| God of War III | 5 | `0x50ddc0` | 0 | 3 | 47 | 8 |
| Minecraft (PS3) | 5 | `0xbbe520` | 0 | 4,097 | 45 | 11 |
| Uncharted 2 | 5 | `0xdd8618` | 0 | 116 | 391 | 7 |
| Demon's Souls | 5 | `0x1916138` | 0 | 4,097 | 134 | 12 |
| Grand Theft Auto: San Andreas | 5 | `0x77b718` | 0 | 4,097 | 23 | 0 |
| Grand Theft Auto V | 5 | `0x1a90b80` | 0 | 4,097 | 56 | 8 |

These are recorded analysis snapshots. OPD and PRX values may come from heuristics and should not be treated as confirmed import/export resolution. Zero detected symbols does not mean the ELF contains no functions.

Raw lift reports: [GOW3](games/GOW3/GOW3_lift_report.txt), [Minecraft](games/Mineacraft/lift_report.txt), [Uncharted 2](games/Uncharted2/UNCHARTED2_lift_report.txt), [Demon's Souls](games/DemonsSouls/DemonsSouls_lift_report.txt), and [Grand Theft Auto: San Andreas](games/Grand%20Theft%20Auto%20-%20San%20Andreas/lift_report.txt), and [Grand Theft Auto V](games/Grand%20Theft%20Auto%20V/lift_report.txt). The matching [San Andreas analysis report](games/Grand%20Theft%20Auto%20-%20San%20Andreas/analysis_report.txt) and [GTA V analysis report](games/Grand%20Theft%20Auto%20V/analysis_report.txt) are archived with their title records. Analysis reports are archived beside each game's record.

## Interpreting the metric

Grand Theft Auto: San Andreas has a recorded snapshot of 1,893,428 instruction instances, all reported translated, 0 reported unimplemented, and 232 chunks. GTA V has a recorded snapshot of 6,934,214 instruction instances, all reported translated, 0 reported unimplemented, and 847 chunks. Its ELF analysis reported 5 PT_LOAD segments, entry OPD `0x1a90b80`, 0 detected symbols, 4,097 OPD entries, 56 PRX/module string hits, and 8 embedded SPU images. These are static report counts only. These reports indicate that the lifter classified every instruction instance in each recorded input snapshot as translated. They do not prove that each emitted operation preserves PowerPC semantics. Approximate operations and no-op fallbacks can still be counted as translated.

Coverage is measured against the input that was analyzed. A different game, firmware revision, executable, or code path may exercise instruction forms absent from a snapshot. Keep these measurements separate:

- **Decode coverage:** whether an instruction encoding is recognized.
- **Translation coverage:** whether the lifter emits translated code for an instruction instance.
- **Semantic correctness:** whether the translated behavior matches the PowerPC specification.
- **Runtime correctness:** whether generated code and runtime services behave correctly.
- **Game compatibility:** whether the complete title operates correctly.

## Developer workflow for opcode changes

1. Inspect lift and analysis reports to identify approximate operations, no-op fallbacks, or instruction families lacking semantic tests.
2. Identify the decoder/lifter path for the opcode and inspect related instruction families.
3. Confirm operand fields, record-bit behavior, condition-register effects, overflow/carry behavior, memory endianness, and exception implications against the PowerPC specification or a trusted reference.
4. Add a focused synthetic regression case that isolates the behavior.
5. Implement the operation without silently replacing unsupported behavior with a no-op.
6. Run the synthetic smoke test and relevant instruction tests.
7. Re-run the relevant game lifts after changes, then compare report counts and generated code.
8. Record actual command results and remaining limitations in the relevant documentation.

## Runtime diagnostics

When guest execution leaves translated code, inspect captured guest PC, LR, CTR, stack pointer, TOC, branch target calculation, indirect branches, function descriptors, and missing translated chunks. Diagnostic values are evidence for investigation, not automatic proof of a root cause.

System calls, PRX imports, SPU execution, RSX graphics, and runtime control flow must be tested separately from PPU translation.


## Publication note

Game titles identify the recorded test inputs only and do not imply affiliation, endorsement, or permission to redistribute game content. Counts are measurements from particular input snapshots, not a compatibility guarantee. Before publishing updated reports or logs, remove private local paths, usernames, secrets, proprietary strings, and unnecessary input-derived data. Do not include game binaries, firmware, keys, extracted assets, or third-party artwork unless its redistribution rights have been verified.
