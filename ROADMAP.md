# PPSX33 implementation status

Not a phase list. Concrete subsystems and how complete they are, based on source and GOW3 run logs (latest checked: 2026-10-27).

Legend: **Done** · **Partial** · **In progress** · **Not started**

## Pipeline

| Implementation | Status | Notes |
| --- | --- | --- |
| ELF64 BE load / segments | Done | Guest image + VA map |
| PPU lift → C++ chunks | Partial | Large unimplemented opcode counts on big titles |
| GUI load / decompile / build | Done | WinForms; MSVC via vswhere |
| Game package layout (SFO/PSARC paths) | Partial | Scan/read helpers; not full FS fidelity |

## Runtime CPU / OS

| Implementation | Status | Notes |
| --- | --- | --- |
| PPU run loop + soft recovery | Partial | Sticky BREAK still frequent on GOW3 |
| VMX / Altivec coverage | Partial | Dispatch present; not full ISA |
| LV2 syscalls (mem, thread, time, event) | Partial | Expanded table; many still return 0 |
| PRX / NID import stubs | Partial | Table + filter; unknown NIDs remain |
| cellSysmodule load by id | Partial | Maps common ids; no full PRX ELF |
| SPU interpreter / MFC / SPURS | Partial | Stubs and coarse ticks |

## Graphics (RSX / GCM)

| Implementation | Status | Notes |
| --- | --- | --- |
| D3D11 device / swapchain / resize | Done | |
| GCM control / labels / local addr | Done | |
| FIFO JUMP/CALL fix + PUT/GET mod | Done | Log no longer shows methods≈131072 |
| Surface COLOR offset/pitch/size | Done | |
| CLEAR solid color to guest surface | Done | |
| BEGIN/END vs DRAW_ARRAYS split | Done | Inject emits 0x1814 |
| Texture unit regs (0x1A00+) | Partial | State recorded; formats limited |
| Vertex array offset/format | Partial | VA0 path; multi-attr weak |
| Host draw of VA / indexed | Partial | Works for inject triangle |
| Guest color blit to window | Partial | Present blits surface without rainbow overwrite |
| SCE VP/FP → HLSL | Partial | Experimental compile path |
| **Title-authored GCM (real meshes)** | **In progress** | Blocked on boot advancing PUT with game methods |
| Depth/stencil/blend accurate | Partial | State bits; not full |
| Tile / zcull / reports | Not started | Stubs only |

## Audio / input / save

| Implementation | Status | Notes |
| --- | --- | --- |
| waveOut path | Partial | |
| Pad map / XInput | Partial | |
| cellSaveData | Partial | |

## UI / packaging

| Implementation | Status | Notes |
| --- | --- | --- |
| DualShock WebView2 mapper | Partial | Full HTML mapper; pad_map.json on build |
| Lift report in GUI console | In progress | Pasted after successful Decompile |
| dist/runtime packaging | Partial | Sources shipped for per-game `ps3rt.dll` MSVC build |

## Current focus

1. Keep FIFO and present honest (no diagnostic overwrite of guest color).
2. Inject and any real methods use DRAW_ARRAYS (`0x1814`) so the host draw path runs.
3. Reduce sticky BREAK / import stalls so GOW3 can submit its own GCM.
4. Expand texture format and multi-attribute vertex fetch when real methods appear.

## Verification

Rebuild game runtime after overlay, run GOW3, confirm log:

- `XV-O CLEAR` without following rainbow `XXV-K paint` every flip
- `inject` followed by draw with `method=0x1814` when flush runs
- `methods` / `packets` stay small (tens), not 64k+

Real GOW menus/meshes require title GCM traffic beyond inject.
