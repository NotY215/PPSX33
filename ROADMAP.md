# PPSX33 Roadmap

**Last updated:** 2026-10-25 (XXV-K/L frame start + wild-pc)  
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

**Current focus:** **XXV-K** guest frames + **XXV-L** wild-pc (pack).

### XXV subphases

| ID | Status | Work |
|----|--------|------|
| **A–J** | DONE (untested) | Through services + E2E checklist |
| **K** | DONE (untested) | Guest color buffer paint → first presentable frames |
| **L** | DONE (untested) | Sticky wild-pc break + LR-first recovery |

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
