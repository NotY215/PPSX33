# PPSX33 Roadmap

**Last updated:** 2026-10-16 (XXIV-A…C in tree)  
**Scope:** Experimental PlayStation 3 static recompiler — research toolkit, not a retail emulator.

---

## PHASE 1 — overview

| Band | Range | Status | Detail |
|------|-------|--------|--------|
| Core pipeline | **I – XIX** | TESTING | [roadmap/I.md](roadmap/I.md) … [roadmap/XIX.md](roadmap/XIX.md) |
| Packaging | **XX-A – XX-G** | DONE | [roadmap/XX-A.md](roadmap/XX-A.md) … |
| Runtime base | **XX-H – XX-O** | DONE | [roadmap/XX-H.md](roadmap/XX-H.md) … |
| Lettered post-XX | **XXI (A – Z)** | DONE | [roadmap/XXI.md](roadmap/XXI.md) |
| Pipeline correctness | **XXII** | DONE | [roadmap/XXII.md](roadmap/XXII.md) |
| Execution reality | **XXIII** | IN TREE (log-reviewed) | [roadmap/XXIII.md](roadmap/XXIII.md) |
| Boot-to-first-pixels | **XXIV** | ACTIVE | [roadmap/XXIV.md](roadmap/XXIV.md) |

**Current focus:** XXIV-A…C (audio fail-once, FIFO truth, HLSL/diagnostic prove). Next: **XXIV-D/E** (PRX NID, COLOR_OFFSET blit).

### XXIII (A–Z — log reviewed)

| Band | Status | Notes |
|------|--------|-------|
| **A–Z** | IN TREE | [roadmap/XXIII.md](roadmap/XXIII.md) |

### XXIV (active)

| ID | Status | Work |
|----|--------|------|
| **XXIV-A** | DONE (untested) | Audio open fail-once |
| **XXIV-B** | DONE (untested) | FIFO bound / no fake method flood |
| **XXIV-C** | DONE (untested) | HLSL compile log + diagnostic frame |
| **XXIV-D** | PLANNED | PRX NID + return-to-LR |
| **XXIV-E** | PLANNED | COLOR_OFFSET blit |

Detail: [roadmap/XXIV.md](roadmap/XXIV.md).

---

## Future — OpenGL fallback

| Item | Plan |
|------|------|
| APIs | OpenGL **3.3** preferred; **2.1** minimum tier for weak GPUs |
| Selection | At build/run: if GPU supports adequate **D3D11** → Direct3D 11; else → **OpenGL** |
| Goal | Usable path on PCs **without** a dedicated GPU |

---

## Requirements

Host minimum and recommended specs: [README.md](README.md#requirements) and the in-app **Requirements** tab.

---

## Related docs

[README.md](README.md) · [USER_MANUAL.md](USER_MANUAL.md) · [LEGAL.md](LEGAL.md)

---

*No proprietary keys or copyrighted game assets are included.*
