# PPSX33 Roadmap

**Last updated:** 2026-10-16 (XXV expanded per GPT audit)  
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

**Current focus:** XXV-D RSX pipeline start (guest surface present); then E/F PPU truth + strict faults; G/H HLE + SPU/DMA.

### XXV subphases (summary)

| ID | Status | Work |
|----|--------|------|
| **A** | DONE (untested) | Audio C3861 / host open |
| **B** | DONE (untested) | Visible host present |
| **C** | PARTIAL | Lifter empty opc honesty |
| **D** | ACTIVE | RSX FIFO / surface / present guest RT |
| **E** | PLANNED | Full lifter status + opcode audit |
| **F** | PLANNED | validate_guest_pc / capture_guest_fault |
| **G** | PLANNED | PRX/LV2 contracts |
| **H** | PLANNED | SPU/MFC/SPURS layers |
| **I** | PLANNED | FS / audio / pad / save services |
| **J** | PLANNED | End-to-end title checklist |

Full function lists and audit table: [roadmap/XXV.md](roadmap/XXV.md).

### GPT audit priorities (accepted)

1. PPU translation truthful
2. Deterministic faults (limit recovery guessing)
3. Real RSX pipeline
4. PRX/LV2 contracts
5. SPU/MFC/SPURS semantics
6. Game services
7. End-to-end proof

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
