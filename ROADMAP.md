# PPSX33 Roadmap

**Last updated:** 2026-10-25 (XXV-H depth + RSX C3861 fix)  
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

**Current focus:** **XXV-H** SPU/MFC/SPURS depth (pack). Next: **XXV-I** game services.

### XXV subphases

| ID | Status | Work |
|----|--------|------|
| **A–E** | DONE (untested) | Audio, present, lifter honesty, RSX host depth |
| **F** | DONE (untested) | Deterministic faults / recovery visibility |
| **G** | DONE (untested) | PRX/LV2 contracts, strict import/LV2 env |
| **H** | DONE (untested) | SPU ISA depth, MFC tag path, SPURS wait; RSX C3861 forward decls |
| **I** | PLANNED | FS / audio / pad / save services |
| **J** | PLANNED | End-to-end title checklist |

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
