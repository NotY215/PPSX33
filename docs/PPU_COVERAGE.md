# PPSX33 PPU Instruction Coverage

## Latest recorded GOW3 lift

| Metric | Result |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated instances reported | 1,285,560 |
| Unimplemented instances reported | 0 |
| Chunks | 157 |
| Static translation coverage | 100% for this report snapshot |
| Runtime/game compatibility | Not established |

Raw counts are stored in [GOW3_lift_report.txt](games/GOW3/GOW3_lift_report.txt). Context is documented in [GOW3.md](games/GOW3/GOW3.md).

## Interpreting the metric

The report indicates that the lifter classified every instruction instance in this input snapshot as translated. It does not prove that each emitted operation preserves PowerPC semantics. Approximate operations and no-op fallbacks can still be counted as translated.

Coverage is measured against the input that was analyzed. A different game, firmware revision, executable, or code path may exercise instruction forms absent from the snapshot. Keep these measurements separate:

- **Decode coverage:** whether an instruction encoding is recognized.
- **Translation coverage:** whether the lifter emits translated code for an instruction instance.
- **Semantic correctness:** whether the translated behavior matches the PowerPC specification.
- **Runtime correctness:** whether generated code and runtime services behave correctly.
- **Game compatibility:** whether the complete title operates correctly.

## Implementation locations

The main PPU translation logic is in `src/core/ppu_lifter.cpp`. Generated-code helpers are in `runtime/ppu_runtime.h`. The synthetic ELF generator is `tests/make_test_elf.py`.

## Developer workflow for opcode changes

1. Identify the decoder/lifter path for the opcode and inspect related instruction families.
2. Confirm operand fields, record-bit behavior, condition-register effects, overflow/carry behavior, memory endianness, and exception implications against the PowerPC specification or a trusted reference.
3. Add a focused synthetic regression case that isolates the behavior.
4. Implement the operation without silently replacing unsupported behavior with a no-op.
5. Run the synthetic smoke test and any relevant instruction tests.
6. Re-run the GOW3 lift when an input is available, then compare report counts and generated code.
7. Record actual command results and remaining limitations in the relevant documentation.

For multi-game analysis, follow the workflow in [Uncharted 2's analysis plan](games/Uncharted2/UNCHARTED2.md).

## Runtime diagnostics

When guest execution leaves translated code, inspect captured guest PC, LR, CTR, stack pointer, TOC, branch target calculation, indirect branches, function descriptors, and missing translated chunks. Diagnostic values are evidence for investigation, not automatic proof of a root cause.

System calls, PRX imports, SPU execution, RSX graphics, and runtime control flow must be tested separately from PPU translation.
