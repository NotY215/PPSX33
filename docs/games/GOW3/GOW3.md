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
| **I** | BOOT | **Done** | ps3rt_init, log file next to EXE |
| **II** | MEM | **Done** | PS3IMG1 segment map, guest_alloc |
| **III** | PPU | **Done** | Lifter 100%, OPD/stub at 0x39800000, progress_hw |
| **IV** | SPU | **Partial** | create/load/run stubs, mailbox; ISA interpreter later |
| **V** | GCM | **Done** | control/labels/FIFO emit, flip NIDs |
| **VI** | RSX present | **Done** | D3D11 window, Present, message pump, force_exit |
| **VII** | RUNNING | **Done** | Stage marker after ~30 flips; throttled log |
| **VIII** | EXIT | **Done** | WM_CLOSE → force_exit / ExitProcess(0) |
| **IX** | Build pipeline | **Done** | BUILD_TOTAL, rsp link files, PATH fix for vcvars |
| **X** | GUI progress | **Done** | N/total, ETA, cancel unlock, live console |
| **XI** | Cleanup | **Done** | Trim codebase/ + input/ post-build |
| **XII** | GCM inject | **Done** | Stall inject clear/scissor when guest put idle |
| **XIII** | Clear surface | **In progress** | Color/depth/stencil mask + DSV clear |
| **XIV** | Draw fidelity | **In progress** | draws counter, depth/stencil state, no pure-blue clear |
| **XV** | Real geometry | **Next** | Vertex/index from FIFO, shaders, textures |
| **XVI** | Game assets | **Later** | Run from game folder, FS HLE for USRDIR |

## Runtime diagnostics (2026-10-13)

From `log_2026-10-13_15-30-09.txt`:

- Stages: BOOT → MEM → RSX → GCM → PPU → SPU → RUNNING
- Flips > 60k, clears > 30k, **draws still 0** until XIV rebuild (inject was pure-blue `0xF1`)
- Window: solid blue was from inject `0xF1` — fixed to cycle dark slate colors
- Close path: force_exit present

## Where to run the EXE

**Now:** `build/dist/projects/<name>/output/` (needs `guest_image.bin` + `ps3rt.dll` beside EXE).

**Later (assets):** copy EXE + `ps3rt.dll` into the game folder next to `EBOOT.BIN`.

## Reproduction

1. Decrypt EBOOT / Load ELF → Decompile → Build → Run from `output/`.
2. Expect dark slate window (not pure blue) after XIV rebuild; log `draws=` should rise.
3. Do not commit ELF, EBOOT, assets, or keys.
