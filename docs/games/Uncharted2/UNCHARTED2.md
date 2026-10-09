# Uncharted 2: Among Thieves PPU Analysis

## Scope

Use this title for static PPU opcode discovery and coverage expansion. No runtime compatibility or playability result is claimed. SPU execution, RSX/GCM, operating-system services, and complete game support are separate workstreams.

## Analysis procedure

1. Identify the game build and use a legally obtained decrypted PS3 ELF.
2. Create a PPSX33 project and run the lift/decompile step.
3. Archive `lift_report.txt` and `analysis_report.txt` in this directory after checking for sensitive paths or unintended extracted content.
4. Record the PPSX33 commit, input identification, date, and exact workflow used.
5. Compare the report with current decoder and lifter behavior. Classify gaps as unrecognized encodings, missing translation, approximate semantics, or runtime dependencies.
6. Prioritize high-frequency gaps while accounting for correctness risk.
7. Implement the opcode in `src/core/ppu_lifter.cpp` and relevant helpers in `runtime/ppu_runtime.h`.
8. Add a focused synthetic regression test when practical. Include record-bit, condition-register, carry/overflow, memory, and endianness effects when relevant.
9. Run the synthetic smoke test and relevant tests. Re-run the lift after changes and update measurements only from actual output.
10. Re-run GOW3 or other existing baselines when input and tool access are available to detect regressions.

## Results

| Metric | Status |
| --- | --- |
| Analysis | Not recorded |
| Instructions | Not measured |
| Translated instances | Not measured |
| Unimplemented instances | Not measured |
| Chunks | Not measured |
| Opcode changes based on this target | None recorded |

Update this table only after running the tool and preserving the corresponding report.

## Artifact policy

Do not commit commercial-game ELFs, EBOOT files, keys, copyrighted assets, or generated game executables. Store only text reports that are appropriate for public release. Static translation counts must not be presented as semantic correctness or game compatibility.

## Related documentation

- [Directory overview](README.md)
- [Game coverage index](../README.md)
- [PPU coverage methodology](../../PPU_COVERAGE.md)
- [Developer guide](../../DEVELOPER_GUIDE.md)
