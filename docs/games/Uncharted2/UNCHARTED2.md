# Uncharted 2: Among Thieves — PPU opcode expansion plan

**Role:** Secondary target for **static PPU opcode coverage** only.

**Do not** build or launch a native `game.exe` for Uncharted 2 until God of War III runtime work is further along. Primary playability target remains GOW3.

## Goals

- Discover instruction families and encodings not exercised (or only approximated) in the GOW3 lift.
- Implement or harden those opcodes in `src/core/ppu_lifter.cpp` and runtime helpers.
- Keep before/after lift statistics under this folder.
- Feed regression tests where a small synthetic ELF can isolate the behavior.

## Workflow

1. Use a legally obtained, decrypted PS3 ELF for Uncharted 2.
2. Create a project and run **Decompile only** (GUI or CLI). Skip Build.
3. Copy `lift_report.txt` and `analysis_report.txt` into `docs/games/Uncharted2/` (versioned snapshots).
4. Sort missing / weak opcodes by frequency; implement the top items.
5. Re-lift Uncharted 2 and GOW3; update tables below only from real tool output.
6. Do not claim game compatibility from static coverage alone.

## Results (fill only from real lifts)

| Metric | Status |
| --- | --- |
| Analysis started | No |
| Instructions | Not measured |
| Translated | Not measured |
| Unimplemented | Not measured |
| Chunks | Not measured |
| Opcode fixes driven by this title | None recorded yet |

## Out of scope for this track

- SPU full execution (Phase 4)
- RSX / GCM (Phase 5)
- Full PRX HLE and playability
- Shipping or running a recompiled Uncharted 2 binary

See [README](README.md), [game index](../README.md), and [ROADMAP](../../../ROADMAP.md).
