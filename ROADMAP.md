# PPSX33 Roadmap

**Last updated:** 2026-10-15  
**Scope:** Experimental PlayStation 3 static recompiler — research toolkit, not a retail emulator.

---

## PHASE 1 — XXII (overview)

| Band | Range | Status | Detail |
|------|-------|--------|--------|
| Core pipeline | **I – XIX** | TESTING | [roadmap/I.md](roadmap/I.md) … [roadmap/XIX.md](roadmap/XIX.md) |
| Packaging | **XX-A – XX-G** | DONE | [roadmap/XX-A.md](roadmap/XX-A.md) … |
| Runtime base | **XX-H – XX-O** | DONE | [roadmap/XX-H.md](roadmap/XX-H.md) … |
| Lettered post-XX | **XXI (A – Z)** | DONE | [roadmap/XXI.md](roadmap/XXI.md) |
| Pipeline correctness | **XXII** (active) | IN PROGRESS | [roadmap/XXII.md](roadmap/XXII.md) |

**Current focus: Phase XXII** — fix real pipeline gaps (RSX, PPU/SPU, LV2/HLE, modules, timing), not more surface-level stubs.

### XXII subphases

| ID | Status | Work |
|----|--------|------|
| **XXII-A** | DONE | RSX present / color-buffer host fidelity |
| **XXII-B** | DONE | PPU soft-recovery / helper surface |
| **XXII-C+** | PLANNED | SPU/MFC, LV2/HLE, modules/FS, timing/audio/pad |

---

## Future — OpenGL fallback

| Item | Plan |
|------|------|
| APIs | OpenGL **3.3** preferred; **2.1** minimum tier for weak GPUs |
| Selection | At build/run: if GPU supports adequate **D3D11** → Direct3D 11; else → **OpenGL** |
| Goal | Usable path on PCs **without** a dedicated GPU |

Not part of XXII-A/B; tracked as a future phase.

---

## Requirements

Host minimum and recommended specs: [README.md](README.md#requirements) and the in-app **Requirements** tab.

---

## Related docs

[README.md](README.md) · [USER_MANUAL.md](USER_MANUAL.md) · [LEGAL.md](LEGAL.md)

---

*No proprietary keys or copyrighted game assets are included.*
