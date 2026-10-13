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
| **XIII** | Clear surface | **Done** | Color/depth/stencil mask + DSV clear |
| **XIV** | Draw fidelity | **Done** | draws counter, depth/stencil state, no pure-blue clear |
| **XV** | Fixed-function | **Done** | Tint overlay on draw, shader/tex tracking |
| **XVI** | Audio | **Next** | cellAudio / libaudio HLE |
| **XVII** | Game assets | **Later** | Run from game folder, FS HLE for USRDIR |

## Runtime diagnostics (2026-10-13)

From `log_2026-10-13_15-49-19.txt` (Phase XIV):

- Stages: BOOT → MEM → RSX → GCM → PPU → SPU → RUNNING
- Flips > 20k, **draws rising with clears**, unknown=0
- Depth+stencil present path active
- Clean EXIT on window close

## Where to run the EXE

**Now:** `build/dist/projects/<name>/output/` (needs `guest_image.bin` + `ps3rt.dll` beside EXE).

**Later (assets):** copy EXE + `ps3rt.dll` into the game folder next to `EBOOT.BIN`.

## Reproduction

1. Decrypt EBOOT / Load ELF → Decompile → Build → Run from `output/`.
2. After XV: log shows `draws=` + `depth=` + `shaders=`; window tint shifts.
3. Do not commit ELF, EBOOT, assets, or keys.
