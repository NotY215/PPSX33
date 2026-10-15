# Phase XXII — Pipeline correctness (ACTIVE)

**Status:** IN PROGRESS  
**Track:** PHASE 1 focus after XXI

Work addresses review findings: incomplete RSX pipeline, PPU/SPU semantics, LV2/HLE stubs, modules/filesystem, timing.

## Subphases

| ID | Name | Status | Work |
|----|------|--------|------|
| **XXII-A** | RSX present / color-buffer fidelity | DONE (this drop) | Prefer guest color/tex upload on present; clearer `[rsx] XXII-A` logs |
| **XXII-B** | PPU soft-recovery surface | DONE (this drop) | Unimplemented ops soft-continue; recovery helpers emphasized |
| **XXII-C** | SPU / MFC fidelity | PLANNED | Channels, DMA lists, atomics, tags |
| **XXII-D** | LV2 / HLE contracts | PLANNED | Fail visibly; implement high-impact syscalls |
| **XXII-E** | Modules / PRX / FS | PLANNED | Reloc, imports, path/errors |
| **XXII-F** | Timing / audio / pad | PLANNED | Coordinated timing; audio ring; pad depth |

## Future (after XXII)

**OpenGL fallback:** OpenGL 3.3 (or 2.1) for machines without a suitable D3D11 GPU. Build/runtime auto-check: if GPU supports D3D11 Feature Level adequately → Direct3D 11; else → OpenGL path. Not implemented in XXII-A/B.

## Related

[ROADMAP.md](../ROADMAP.md) · [XXI.md](XXI.md)
