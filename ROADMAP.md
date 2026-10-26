# PPSX33 Roadmap

**Last updated:** 2026-10-26  
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
| Host UX | **XXVI** | ACTIVE | Native WinForms UI (WebView2 removed) |

**Current focus:** Guest-driven frames (XXV) + native host UI (XXVI).

### XXV subphases

| ID | Status | Work |
|----|--------|------|
| **A–J** | DONE (untested) | Services + E2E checklist |
| **K** | DONE (untested) | Guest color buffer → presentable frames |
| **L** | DONE (untested) | Sticky wild-pc + LR-first recovery |
| **M+** | PLANNED | Real RSX geometry / shader IR (no diagnostic-only draws) |

### XXVI — Native UI (host)

| ID | Status | Work |
|----|--------|------|
| **A** | DONE | Drop WebView2; pure WinForms shell |
| **B** | DONE | Website palette, logo PNG, animated ring, workflow rail |
| **C** | IN TREE | Owner-draw tabs, status/progress, Locate RPCS3 |
| **D** | PLANNED | Deeper motion / reduced-overdraw polish |

Detail: [roadmap/XXV.md](roadmap/XXV.md) · [roadmap/XXVI.md](roadmap/XXVI.md)

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
