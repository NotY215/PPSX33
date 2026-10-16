# Phase XXIII — Execution reality (ACTIVE)

**I–W in tree; testing of I+ still pending.**

## Subphases

| ID | Status | Focus |
|----|--------|-------|
| **A–H** | DONE | Boot surface, present, audio, lifter, SPU/LV2/PRX |
| **I–N** | DONE (untested) | FS, SPURS, timing, formats, FIFO, vertex track |
| **O–Q** | DONE (untested) | Programs/tex/HLE NIDs |
| **R–S** | DONE (untested) | Control-flow TOC/OPD; thread create |
| **T** | DONE (untested) | **Runtime HLSL VS/PS blit** (D3DCompile) for textured present/draw |
| **U** | DONE (untested) | DXT/BC format detect + linear fallback upload |
| **V** | DONE (untested) | Host textured quad on guest draw methods |
| **W** | DONE (untested) | Save/sysutil surface tags |
| **X** | PLANNED | Per-title golden frame tests |
| **Y** | PLANNED | Full RSX shader ISA → HLSL (not only blit) |
| **Z** | PLANNED | Tiled / swizzled memory + full BC decode |

## T scope

T compiles a **real HLSL** vertex+pixel pair for fullscreen textured blit and uses it on draw/present when a guest texture/color buffer is uploaded. It is **not** yet a full per-shader RSX microcode translator (that is **Y**).

## When full game renders

Still requires guest FIFO PUT activity, stable PPU (R), and Y/Z-class work for arbitrary shaders/tiling. First non-solid frame: methods ≫ 4 and HLSL path logs `[rsx] XXIII-T/V`.

## Build fix

`assets.cpp`: `guest_path` undeclared in SELF header probe — fixed to use host path in miss log.
