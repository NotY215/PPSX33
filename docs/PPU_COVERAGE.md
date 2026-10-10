# PPSX33 PPU Instruction Coverage

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.svg" alt="PPSX33 circular logo" width="120" /></p>


## Recorded static-lift snapshots

| Game | Instruction instances | Translated instances reported | Unimplemented instances reported | Chunks | Static translation coverage |
| --- | ---: | ---: | ---: | ---: | --- |
| God of War III (GOW3) | 1,285,560 | 1,285,560 | 0 | 157 | 100% for this snapshot |
| Uncharted 2: Among Thieves | 3,485,016 | 3,485,016 | 0 | 426 | 100% for this snapshot |

The GOW3 raw counts are stored in [GOW3_lift_report.txt](games/GOW3/GOW3_lift_report.txt). Uncharted 2's generated reports are currently in the local project at `E:\\PPSX33\\build\\dist\\UNCHARTED 2\\codebase\\lift_report.txt` and `analysis_report.txt`; they have not been archived in this repository. See the [Uncharted 2 analysis record](games/Uncharted2/UNCHARTED2.md).

## Interpreting the metric

These reports indicate that the lifter classified every instruction instance in each recorded input snapshot as translated. They do not prove that each emitted operation preserves PowerPC semantics. Approximate operations and no-op fallbacks can still be counted as translated.

Coverage is measured against the input that was analyzed. A different game, firmware revision, executable, or code path may exercise instruction forms absent from a snapshot. Keep these measurements separate:

- **Decode coverage:** whether an instruction encoding is recognized.
- **Translation coverage:** whether the lifter emits translated code for an instruction instance.
- **Semantic correctness:** whether the translated behavior matches the PowerPC specification.
- **Runtime correctness:** whether generated code and runtime services behave correctly.
- **Game compatibility:** whether the complete title operates correctly.

## Uncharted 2 analysis metadata

The recorded analysis reported 5 ELF load segments, entry OPD `0xdd8618`, 0 symbols, 116 OPDs, 391 PRX heuristic hits, and 7 embedded SPU images. Zero symbols does not mean zero functions; OPD/function discovery can use other analysis heuristics. PRX hits are heuristic findings, and SPU image detection does not establish SPU execution support.

## Implementation locations

The main PPU translation logic is in `src/core/ppu_lifter.cpp`. Generated-code helpers are in `runtime/ppu_runtime.h`. The synthetic ELF generator is `tests/make_test_elf.py`.

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
