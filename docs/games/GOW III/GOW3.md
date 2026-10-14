# God of War III (GOW3): Lift and runtime record

## Static lift statistics

| Metric | Count |
| --- | ---: |
| Instruction instances | 1,285,560 |
| Translated | 1,285,560 |
| Unimplemented | 0 |
| Chunks | 157 |
| Segments | 5 |
| Entry OPD | 0x50ddc0 |
| Symbols | 0 |
| OPD entries (heuristic) | 4097 |
| PRX string hits | 23–47 |
| Embedded SPU images | 8 |
| Reported static coverage | 100% |

## Roadmap (Roman phases)

| Phase | Name | Status | Notes |
| --- | --- | --- | --- |
| **I–XIII** | Boot→Clear | **Done** | See root ROADMAP.md |
| **XIV** | Draw fidelity | **Done** | depth/stencil, no pure-blue |
| **XV** | Fixed-function | **Done** | Tint on draw, shader/tex counters |
| **XVI** | Audio | Pending | cellAudio HLE |
| **XVII** | Pad / input | **Done** | XInput + keyboard → cellPad |
| **XVIII+** | Save / perf / playable | Pending | |

## Runtime diagnostics (2026-10-13 16:07)

From `log_2026-10-13_16-07-55.txt` (Phase XV):

- Stages complete; RUNNING after flip ~32
- draws / clears / depth / stencil all rising together
- unknown methods = 0
- Clean EXIT on window close

## Input map (Phase XVII)

| Host | CELL_PAD |
|------|----------|
| WASD / Arrows | D-pad + left stick |
| Enter / K | Cross |
| Esc / L | Circle |
| Space / I | Triangle |
| Backspace / J | Square |
| Q / E | L1 / R1 |
| Z / C | L2 / R2 |
| Tab / Shift | Start / Select |
| Xbox pad | Full XInput → PS3 map |

## Where to run

`build/dist/projects/<name>/output/` (guest_image.bin + ps3rt.dll beside EXE).
