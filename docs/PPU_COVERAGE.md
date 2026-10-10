# PPSX33 PPU Instruction Coverage

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="120" /></p>


## Recorded static-lift snapshots

| Game | Instruction instances | Translated instances reported | Unimplemented instances reported | Chunks | Static translation coverage |
| --- | ---: | ---: | ---: | ---: | --- |
| God of War III (GOW3) | 1,285,560 | 1,285,560 | 0 | 157 | 100% for this snapshot |
| Uncharted 2: Among Thieves | 3,485,016 | 3,485,016 | 0 | 426 | 100% for this snapshot |
| Demon's Souls | 6,343,506 | 6,343,506 | 0 | 775 | 100% for this snapshot |

## ELF and analysis metadata

| Game | PT_LOAD segments | Entry OPD | Symbols | OPD entries | PRX/module string hits | Embedded SPU images |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| God of War III | 5 | `0x50ddc0` | 0 | 3 | 47 | 8 |
| Uncharted 2 | 5 | `0xdd8618` | 0 | 116 | 391 | 7 |
| Demon's Souls | 5 | `0x1916138` | 0 | 4,097 | 134 | 12 |

These are recorded analysis snapshots. OPD and PRX values may come from heuristics and should not be treated as confirmed import/export resolution. Zero detected symbols does not mean the ELF contains no functions.

Raw lift report: [Demon's Souls report](games/DemonsSouls/DemonsSouls_lift_report.txt). The GOW3 report is linked from its [game record](games/GOW3/GOW3.md). Uncharted 2's generated `lift_report.txt` and `analysis_report.txt` remain in its local project directory unless separately archived; see the [Uncharted 2 analysis record](games/Uncharted2/UNCHARTED2.md).

## Interpreting the metric

These reports indicate that the lifter classified every instruction instance in each recorded input snapshot as translated. They do not prove that each emitted operation preserves PowerPC semantics. Approximate operations and no-op fallbacks can still be counted as translated.

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
