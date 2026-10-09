# Uncharted 2: Among Thieves PPU Analysis Plan

This document tracks the planned use of *Uncharted 2: Among Thieves* to expand PPSX33's PPU opcode coverage.

## Planned workflow

1. Select and analyze the appropriate decrypted PS3 ELF.
2. Generate a static lift report and preserve the exact report snapshot.
3. Compare encountered PPU instructions with current decoder and lifter support.
4. Prioritize missing or incomplete opcode implementations based on evidence from the report.
5. Add unit or regression tests for each implemented behavior.
6. Regenerate the report and record before-and-after counts.
7. Validate semantics independently where possible. A translated instruction count alone is not proof of correctness.

## Results

| Metric | Status |
| --- | --- |
| Analysis started | No |
| Instructions analyzed | Not measured |
| Translated instructions | Not measured |
| Unimplemented instructions | Not measured |
| Chunks | Not measured |
| Opcode fixes from this target | None recorded |

These fields must be updated only from actual tool output. Do not infer coverage values before the analysis has been run.

## Scope

The immediate purpose is to discover and implement additional PPU opcode behavior. SPU execution, RSX/GCM graphics, operating-system services, and overall game compatibility are separate workstreams and must not be represented as solved by improved PPU coverage alone.
