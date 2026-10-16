# PPSX33 Roadmap

**Last updated:** 2026-10-16 (GOW log analysis; XXIII A–Z in tree)  
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
| Boot-to-first-pixels | **XXIV** | NEXT | See XXIII.md “Next patches” |

**Current focus:** Fix blockers from GOW log `2026-10-16_14-13-47` (audio spam, FIFO put=0x20, missing HLSL proof, weak PRX NID) before more feature surface.

### XXII (finished)

| ID | Status | Work |
|----|--------|------|
| **XXII-A…L** | DONE | [roadmap/XXII.md](roadmap/XXII.md) |

### XXIII (A–Z in tree — log reviewed)

| Band | Status | Notes |
|------|--------|-------|
| **A–Z** | IN TREE | Detail + log table: [roadmap/XXIII.md](roadmap/XXIII.md) |
| Evidence | Mixed | Flips + GCM up; **no game frame**; **audio fail storm** |

### XXIV (next — recommended)

| ID | Priority | Work |
|----|----------|------|
| **XXIV-A** | P0 | Audio: waveOutOpen fail-once, no per-tick retry/log |
| **XXIV-B** | P0 | FIFO: no fake 131k methods when put=get=0x20 |
| **XXIV-C** | P0 | HLSL/diagnostic: one success/fail log + forced gradient if tex=0 |
| **XXIV-D** | P1 | PRX NID quality + import return-to-LR |
| **XXIV-E** | P2 | COLOR_OFFSET blit every flip when present |

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
