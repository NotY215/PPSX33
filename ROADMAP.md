# PPSX33 Roadmap

**Last updated:** 2026-10-25 (XXV-F full depth + requirements recalculated)  
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

**Current focus:** **XXV-F** deterministic faults (pack). Next: **XXV-G** PRX/LV2 contracts.

### XXV subphases

| ID | Status | Work |
|----|--------|------|
| **A–E** | DONE (untested) | Audio, present, lifter honesty, RSX host depth |
| **D** | DONE (untested) | Full depth FIFO/surface/host draw (on E) |
| **F** | DONE (untested) | `validate_guest_pc`, `capture_guest_fault`, recovery logs, storm halt, `PS3RT_STRICT_FAULT=1` |
| **G** | PLANNED | PRX/LV2 contracts |
| **H** | PLANNED | SPU/MFC/SPURS layers |
| **I** | PLANNED | FS / audio / pad / save |
| **J** | PLANNED | End-to-end title checklist |

Detail: [roadmap/XXV.md](roadmap/XXV.md).

### Requirements

Recalculated for **running generated games** vs **lift/build**: [README.md](README.md#requirements).

---

## Future — OpenGL fallback

| Item | Plan |
|------|------|
| APIs | OpenGL **3.3** preferred; **2.1** minimum |
| Selection | D3D11 if capable, else OpenGL |
| Goal | PCs without a dedicated GPU |

---

*No proprietary keys or copyrighted game assets are included.*
