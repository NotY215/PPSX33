# PPSX33 Roadmap

**Last updated:** 2026-10-25 (XXV-I/J services full depth)  
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

**Current focus:** **XXV-I/J** game services (pack). Next: **XXV-K** guest RSX geometry, **XXV-L** wild-pc stability.

### XXV subphases

| ID | Status | Work |
|----|--------|------|
| **A–H** | DONE (untested) | Lifter, RSX host, faults, PRX/LV2, SPU/MFC |
| **I** | DONE (untested) | cellFs depth, audio keepalive, pad poll, save lifecycle |
| **J** | DONE (untested) | End-to-end service checklist log |
| **K** | PLANNED | Guest RSX geometry (non-black surfaces, shaders) |
| **L** | PLANNED | Boot stability (wild-pc / import path) |

Detail: [roadmap/XXV.md](roadmap/XXV.md).

### Requirements

[README.md](README.md#requirements) · [redist/README.txt](redist/README.txt)

---

## Future — OpenGL fallback

| Item | Plan |
|------|------|
| APIs | OpenGL **3.3** preferred; **2.1** minimum |
| Selection | D3D11 if capable, else OpenGL |
| Goal | PCs without a dedicated GPU |

---

*No proprietary keys or copyrighted game assets are included.*
