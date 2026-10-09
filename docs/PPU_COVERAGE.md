# PPSX33 PPU Instruction Coverage

## Latest GOW3 lift snapshot

| Metric | Result |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated / implemented in report | 1,285,560 |
| Unimplemented in report | 0 |
| Chunks | 157 |
| Reported static translation coverage | 100% |
| Runtime/game compatibility | Not established |

The source report is stored at [`docs/games/GOW3/GOW3_lift_report.txt`](games/GOW3/GOW3_lift_report.txt). The detailed game record is [`docs/games/GOW3/GOW3.md`](games/GOW3/GOW3.md).

## What the count means

The report records that the lifter classified every instruction instance in this particular snapshot as translated. It does not prove that each emitted operation preserves PowerPC semantics. Some long-tail instructions are approximate or no-op fallbacks, so “zero unimplemented” must not be interpreted as “every instruction is correctly implemented.”

Other games may exercise instructions, instruction encodings, code paths, or runtime dependencies absent from this snapshot. Add focused regression tests and additional game samples rather than generalizing one lift to the whole PowerPC ISA.

## Current implementation areas

The implementation lives primarily in `src/core/ppu_lifter.cpp` and the generated-code helpers in `runtime/ppu_runtime.h`.

- Integer arithmetic, logical operations, shifts/rotates, and common loads/stores
- Branches, condition-register helpers, and LR/CTR access
- Selected floating-point operations and FPSCR-related handling
- Selected VMX/AltiVec loads, stores, logic, splat, and approximate fallback paths
- Baseline reservation fields for atomic instruction families

These are broad implementation areas, not a claim of complete ISA correctness.

## Runtime diagnostics

The runtime records diagnostic state such as guest PC, LR, CTR, stack pointer, and TOC when execution leaves the translated range. Investigate branch target calculations, indirect branches, missing translated chunks, function descriptors/OPD handling, and return behavior. Diagnostics are clues, not a definitive root-cause report.

PPU translation is only one requirement. System calls, PRX imports, SPU execution, RSX graphics, and runtime control flow must be validated separately.
