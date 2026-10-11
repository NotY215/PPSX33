# PPSX33 Runtime Architecture

This directory contains guest execution support used by generated host builds. The runtime is experimental and does not provide complete PlayStation 3 system, SPU, or RSX compatibility.

## PPU runtime

| File | Responsibility |
| --- | --- |
| `ppu_runtime.h` | Include wrapper for the core and run loop |
| `ppu_runtime_core.h` | Shared PPU context and common definitions |
| `ppu_runtime_core_a.h` | Core runtime helper definitions |
| `ppu_runtime_core_b.h` | Additional memory, XER, condition-register, and FPSCR helpers |
| `ppu_runtime_run.h` | Chunk dispatch, external-call recovery, stack scanning, and null-loop escape handling |

All split headers are required together. The project build driver in `src/core/project.cpp` stages runtime files into generated project output. Update that copy list whenever a required runtime file is added, removed, or renamed.

The execution loop includes experimental recovery behavior for calls that leave known translated chunks. Phase G adds a bounded list of exhausted link-register targets and deeper stack scanning to help escape repeated null-call loops. These heuristics may redirect execution and must not be treated as proof of correct control flow.

## Guest system runtime

- `ps3rt.cpp` provides guest memory initialization and mapping, basic LV2/HLE paths, SPU API foundations, and partial `cellGcmSys` state.
- GCM support includes control-register and label state allocation and handlers for selected control, initialization, flip-status, and main-memory mapping operations.
- `nid_table.h` contains NID-related definitions used by the runtime.
- `boot_continue.patch` is a runtime support patch staged with the package.

The GCM state and handlers are partial HLE implementations. They do not establish compatibility with all PS3 system libraries or titles.

## SPU and RSX

SPU context allocation, image loading, run/stop entry points, mailbox operations, and related MFC paths exist, but this does not imply complete SPU ISA, vector/floating-point, scheduling, or SPURS support. RSX code provides partial FIFO/method and surface/draw/control handling with D3D10/D3D11 integration. It is not a complete RSX command processor or renderer.

The current [roadmap](../ROADMAP.md) marks Phase G complete and Phase H as the next milestone for SPU execution and RSX command draining.

## Validation

Run the synthetic pipeline smoke test after runtime changes:

- Windows: `scripts\run_smoke_test.bat`
- Linux/macOS/compatible MSYS: `bash scripts/run_smoke_test.sh`

The synthetic test validates the pipeline on its test input only. It does not prove commercial-game boot, rendering, or playability.
