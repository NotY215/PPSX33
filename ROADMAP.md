# PPSX33 Roadmap

**Last updated:** 2026-10-22 (XXV-D full depth + XXV-E patches)  
**Scope:** Experimental PlayStation 3 static recompiler — research toolkit, not a retail emulator.

---

## PHASE overview

| Band | Range | Status | Detail |
|------|-------|--------|--------|
| Core pipeline | **I – XIX** | TESTING | [roadmap/I.md](roadmap/I.md) … |
| Packaging / runtime base | **XX – XXII** | DONE | [roadmap/XXII.md](roadmap/XXII.md) |
| Execution reality | **XXIII** | IN TREE | [roadmap/XXIII.md](roadmap/XXIII.md) |
| Boot-to-first-pixels | **XXIV** | A–E in tree | [roadmap/XXIV.md](roadmap/XXIV.md) |
| Truthful exec & pixels | **XXV** | ACTIVE | [roadmap/XXV.md](roadmap/XXV.md) |

**Current focus:** Apply **XXV-E** then **XXV-D full depth** (FIFO / surfaces / host draw / vertex notes). Next: **XXV-F** strict guest faults.

### XXV subphases (summary)

| ID | Status | Work |
|----|--------|------|
| **A** | DONE (untested) | Audio C3861 / host open |
| **B** | DONE (untested) | Visible host present |
| **C** | DONE (untested) | Lifter empty opc → not Implemented |
| **D** | DONE (untested) | Full depth part1+2 on E: FIFO decode, surface bind, host draw, solid triangle, vertex/depth/blend notes, GCM color seed |
| **E** | DONE (untested) | Lifter honesty (unsupported → false + report); present order fix; solid triangle |
| **F** | PLANNED | validate_guest_pc / capture_guest_fault |
| **G** | PLANNED | PRX/LV2 contracts |
| **H** | PLANNED | SPU/MFC/SPURS layers |
| **I** | PLANNED | FS / audio / pad / save services |
| **J** | PLANNED | End-to-end title checklist |

**Patch order:** `PPSX33_XXV_E_render_lifter.zip` → `PPSX33_XXV_D_full_depth_on_E.zip`.

Detail: [roadmap/XXV.md](roadmap/XXV.md).

### Features landed in XXV-D/E (host path)

| Feature | Notes |
|---------|--------|
| Clear → draw → Present | Fixed wipe-after-draw present order |
| Guest color / tex blit | HLSL textured quad when surface bound |
| Solid triangle fallback | GPU path when no guest surface |
| FIFO `state_apply` / `fifo_decode_word` | Method table + draw methods trigger host draw |
| Vertex / depth / blend recording | Part2 notes — not full geometry ISA |
| GCM color seed | Pattern + `set_color_surface` when PUT stuck |
| Lifter unsupported report | Unknown opc/xo31 not counted Implemented |

Still **not** claimed: full RSX vertex/fragment program translation or commercial-title playability.

### GPT audit priorities (accepted)

1. PPU translation truthful — **E started**
2. Deterministic faults — **F next**
3. Real RSX pipeline — **D depth host path in tree**
4. PRX/LV2 contracts — G
5. SPU/MFC/SPURS semantics — H
6. Game services — I
7. End-to-end proof — J

---

## Future — OpenGL fallback

| Item | Plan |
|------|------|
| APIs | OpenGL **3.3** preferred; **2.1** minimum |
| Selection | D3D11 if capable, else OpenGL |
| Goal | PCs without a dedicated GPU |

---

## Requirements

[README.md](README.md#requirements) and in-app **Requirements** tab.

---

*No proprietary keys or copyrighted game assets are included.*
