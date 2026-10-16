# PPSX33 Roadmap

**Last updated:** 2026-10-16 (XXV start: truthful lift + visible frames)  
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

**Current focus:** XXV — fix build (audio), force visible presents, stop silent lifter success; then real RSX surfaces and strict PPU faults.

### XXIV (boot assist)

| ID | Status |
|----|--------|
| A–E | IN TREE — audio fail-once, FIFO bound, HLSL, PRX filter, COLOR/diagnostic |

### XXV (active)

| ID | Status | Work |
|----|--------|------|
| **XXV-A** | DONE (untested) | `audio_queue_s16` forward declare (C3861) |
| **XXV-B** | DONE (untested) | Visible frame present path |
| **XXV-C** | DONE (untested) | Lifter empty opc → not Implemented |
| **XXV-D…H** | PLANNED | RSX surfaces, LiftStatus, strict faults, HLE, SPU/DMA |

Detail: [roadmap/XXV.md](roadmap/XXV.md).

### External audit (GPT) — accepted priorities

1. PPU lifter truthfulness  
2. Deterministic faults (limit wild-PC guessing)  
3. Real RSX pipeline (not diagnostic-only)  
4. LV2/PRX contracts  
5. SPU/MFC/SPURS semantics  

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
