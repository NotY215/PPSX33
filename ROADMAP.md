# PPSX33 Roadmap

This is the **working plan**. Status comes from source and GOW3 run logs, not marketing.
External AI audits are **leads only**: accept what matches code and logs.

Legend: **Done** · **Partial** · **In progress** · **Blocked** · **Not started**

---

## How to read this document

1. **Facts** = observed in logs or source.
2. **Inferences** = likely causes.
3. **Actions** = next engineering steps.
4. A diagnostic triangle is not game rendering.

---

## Current evidence (GOW3)

| Observation | Status |
| --- | --- |
| Lift: translated count can match instruction count (one run: unimplemented 0) | Fact |
| D3D11 window, flips advance | Fact |
| GCM control / labels / local alloc | Fact |
| FIFO method counts small after JUMP fix | Fact |
| Draws still inject/seed; shaders=0 in logs | Fact |
| Sticky BREAK / wild-pc recovery | Fact |
| PRX UNKNOWN NIDs (reduced, not eliminated) | Fact |
| Title meshes / textures / menus on screen | **Not observed** |

**Inference:** Host present and HLE skeleton work; title GCM and boot control flow are incomplete.

---

## GUI controller (native C#)

| Item | Status | Notes |
| --- | --- | --- |
| Replace controller WebView2 page with WinForms/GDI+ | Done | Native `ControllerControl.cs`; HTML page removed |
| Controller shell, buttons, D-pad, shoulders and labels | Implemented | Custom gradient drawing and surface details |
| Analog stick movement and pulsing glow | Implemented | Timer-driven animation with aspect-ratio-safe scaling |
| Keyboard rebinding, reset, JSON import/export | Implemented | Existing `KeyMap.cs` defaults retained |
| Persist settings and write `output/pad_map.json` | Implemented | Save action integrated with GUI |
| Release build and manual interaction/DPI test | Pending | Run local `dotnet publish`; test resize, rebind/cancel, import/export and build-lock |
## A. Host product / packaging

| Item | Status | Action |
| --- | --- | --- |
| GUI load / decompile / build | Done | |
| Prebuilt `ps3rt.dll` / `.lib` | Partial | CMake builds DLL |
| Game build must not re-link empty DLL | **In progress** | Skip `LINK ps3rt.dll` when prebuilt; link EXE to `ps3rt.lib` only |
| `dist/runtime` without `.cpp` | Done | Sources stay in repo |
| `dist/runtime` without `.md` | In progress | Do not copy README |
| Headers in `dist/runtime` | **Required** | See below |
| `redist/` Visual Studio notes | Done | Native WinForms GUI has no browser runtime dependency |
| No bundled compiler tree | Done | System VS |

### Why headers cannot all disappear into the DLL

Lifted game `.cpp` files `#include` `ppu_runtime.h`, `ppu_runtime_run.h`, `nid_table.h`, and related `.inl` content at **compile time**.

A DLL holds **compiled** code, not include text. Options:

1. Ship headers next to the prebuilt DLL (current correct model).
2. Change the lifter so generated code does not include those headers (large design change).

Amalgamating headers into fewer `.h` files is optional cleanup; it does not remove the need for headers.

---

## B. PPU / lift / execution

| Item | Status | Action |
| --- | --- | --- |
| ELF load | Done | |
| Lift to chunks | Partial | Title-dependent |
| VMX / complex ops | Partial | Validate against faults |
| Wild-pc / sticky BREAK | Partial | Trace; reduce silent recovery |
| TOC / OPD | Partial | |

**AI lead:** truthful PPU translation — **valid**. Prefer log-driven fixes over full rewrites.

---

## C. LV2 / PRX / modules

| Item | Status | Action |
| --- | --- | --- |
| Syscalls | Partial | Expand from failing paths |
| NID table | Partial | Add NIDs from **log UNKNOWN lines** |
| cellGcmSys HLE | Partial | |
| sysmodule / fs / game / sysutil | Partial | |
| Full PRX ELF | Partial | |

---

## D. RSX / GCM / rendering (priority)

| Item | Status | Action |
| --- | --- | --- |
| D3D11 present | Done | |
| FIFO JUMP/CALL | Done | |
| Surface / CLEAR | Done | |
| BEGIN / DRAW_ARRAYS | Done | |
| Present without diagnostic overwrite | Done | |
| Inject seed triangle | Partial | Diagnostic only |
| Multi-attr VA / textures | Partial | |
| SCE VP/FP HLSL | Partial | |
| **Title GCM stream** | **Blocked** | Needs boot/HLE so PUT carries game methods |
| Tile / zcull | Not started | |

### Rendering truth ladder

1. Non-inject methods in log (not fixed inject packet counts).
2. VA/IB/tex from game memory.
3. Frames differ from seed triangle.
4. Shader binds affect output.
5. Menu / gameplay stable frames.

**AI lead:** diagnostic draws can hide missing geometry — **correct**.

---

## E. SPU / MFC / SPURS

| Item | Status |
| --- | --- |
| Coarse interpreter | Partial |
| DMA / SPURS depth | Partial / later |

---

## F. Audio / pad / save

| Item | Status |
| --- | --- |
| waveOut / pad / save | Partial |
| Native C# controller mapper | Done | GDI+ drawing, animated sticks/glow, key rebinding, JSON import/export; no WebView2 |

---

## Recommended order

1. **Packaging** — prebuilt DLL path: never link empty `ps3rt.dll`; only game objects + `ps3rt.lib`.
2. **Boot / NID** — sticky BREAK + top UNKNOWN NIDs from latest log.
3. **Guest GCM proof** — first non-inject DRAW.
4. **Textures / multi-attr** when methods appear.
5. **SPU** after interactive frames exist.

---

## Verification

- [ ] Host `ps3rt.dll` links cleanly
- [ ] Game log: `PREBUILT ps3rt.dll (skip link)` then successful `LINK <game>.exe`
- [ ] `dist/runtime`: dll, lib, headers/inl only
- [ ] No nid=0 UNKNOWN spam
- [ ] Later: non-inject DRAW in log

---

## Shared ChatGPT link

The share URL did not expose full message text without login here. Themes from prior audits are included only where they match PPSX33 evidence.
