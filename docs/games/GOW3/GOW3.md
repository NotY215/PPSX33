# God of War III (GOW3): Lift Record

## Recorded lift statistics

| Metric | Count |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated instances reported | 1,285,560 |
| Unimplemented instances reported | 0 |
| Chunks | 157 |
| Reported static translation coverage | 100% for this snapshot |

Raw totals are available in [GOW3_lift_report.txt](GOW3_lift_report.txt).

## Interpretation

This snapshot classifies all 1,285,560 instruction instances as translated. It is a static-lifting measurement for one input and report. It does not prove that every emitted operation matches PowerPC semantics, or establish successful boot, correct rendering, or playability. Some less common operations may use approximate handling or no-op fallbacks.

## Reproduction and regression workflow

1. Use an authorized decrypted ELF matching the analyzed build.
2. Run the lift using the current PPSX33 toolchain.
3. Archive the generated lift and analysis reports with tool revision and input identification.
4. Compare counts and inspect changes in translated code.
5. Run focused synthetic tests for modified opcode semantics.
6. Validate runtime behavior independently and record exact results.

Do not commit the ELF, EBOOT, game assets, keys, or other copyrighted game files. PPU semantics, runtime control flow, system calls, PRX imports, SPU execution, and RSX graphics require separate validation. See [PPU coverage](../../PPU_COVERAGE.md).
