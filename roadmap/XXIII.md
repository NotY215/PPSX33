# Phase XXIII — Execution reality (ACTIVE)

GOW III still shows flips with synthetic methods only until guest GCM PUT advances. **I–N implemented in tree (not fully tested).** **O–Q** add shader/texture/HLE surface depth.

## Subphases

| ID | Status | Focus |
|----|--------|-------|
| **XXIII-A…H** | DONE | Null-PC, present, audio, lifter, RSX methods, SPU/MFC, LV2, PRX NID |
| **XXIII-I** | DONE (untested) | cellFs / PSARC miss logging |
| **XXIII-J** | DONE (untested) | SPURS job slice |
| **XXIII-K** | DONE (untested) | Timing / GET=PUT assist |
| **XXIII-L** | DONE (untested) | Format identity + open coverage |
| **XXIII-M** | DONE (untested) | FIFO drain / GET commit |
| **XXIII-N** | DONE (untested) | Vertex attr tracking |
| **XXIII-O** | DONE (untested) | VP/FP program bind surface + logs |
| **XXIII-P** | DONE (untested) | Texture pitch / 16→32 expand upload |
| **XXIII-Q** | DONE (untested) | Extra GCM/sysutil NIDs; GCM ensure on import |
| **XXIII-R** | PLANNED | PPU control-flow (wild-pc / TOC / OPD) |
| **XXIII-S** | PLANNED | Threading: workers that fill FIFO |
| **XXIII-T** | PLANNED | Full host shader translation (RSX→HLSL/SPIR-V) |
| **XXIII-U** | PLANNED | DXT/BC / tiled texture decode |
| **XXIII-V** | PLANNED | Real indexed draw execution on D3D11 |
| **XXIII-W** | PLANNED | Save/trophy/sysutil depth |
| **XXIII-X** | PLANNED | Per-title boot harness + frame golden tests |

## When can the whole game render?

Not with O–Q alone. End-to-end scene rendering needs **all** of:

1. **Stable PPU control flow** (R) — fewer `wild-pc 0x0` / wrong TOC
2. **HLE that reaches draw setup** (Q+) — imports that init RSX/FIFO must be real enough
3. **Guest FIFO PUT ≠ 0** (M depth) — real command stream, not inject-only
4. **Vertex + shader + texture path** (N/O/P → T/U/V) — actual GPU work
5. **SPU/FS/assets** delivering meshes/textures on time (I/J + formats)

**Practical checkpoint:** first non-solid frame = guest methods ≫ 4, `tex>0` or real color buffer, overlay `draws>0` without inject-only pattern.
**Full game:** only after R–V class work is validated on a title boot path — substantial engineering, not one lettered drop.

## Asset reading vs playing

Open/read for many container types (L) is **necessary but not sufficient**. Play needs correct **bytes at the right guest addresses at the right time** (decrypt EDAT where legal keys exist, decompress archives, feed RSX). Encrypted `.edat`/`.sdat` remain opaque without keys.

## Log tags

- `[rsx] XXIII-O` — VP/FP bind
- `[rsx] XXIII-P` — texture upload
- `[prx] XXIII-Q` — unknown NID / GCM ensure
