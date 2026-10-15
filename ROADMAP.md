# PPSX33 Roadmap

**Last updated:** 2026-10-15 (XXII finished; XXIII active)  
**Scope:** Experimental PlayStation 3 static recompiler — research toolkit, not a retail emulator.

---

## PHASE 1 — XXIII (overview)

| Band | Range | Status | Detail |
|------|-------|--------|--------|
| Core pipeline | **I – XIX** | TESTING | [roadmap/I.md](roadmap/I.md) … [roadmap/XIX.md](roadmap/XIX.md) |
| Packaging | **XX-A – XX-G** | DONE | [roadmap/XX-A.md](roadmap/XX-A.md) … |
| Runtime base | **XX-H – XX-O** | DONE | [roadmap/XX-H.md](roadmap/XX-H.md) … |
| Lettered post-XX | **XXI (A – Z)** | DONE | [roadmap/XXI.md](roadmap/XXI.md) |
| Pipeline correctness | **XXII** | DONE | [roadmap/XXII.md](roadmap/XXII.md) |
| Execution reality | **XXIII** (active) | IN PROGRESS | [roadmap/XXIII.md](roadmap/XXIII.md) |

**Current focus: Phase XXIII** — null-PC recovery, present path, audio keep-alive; deeper P0 work listed in XXIII.md.

### XXII (finished)

| ID | Status | Work |
|----|--------|------|
| **XXII-A…L** | DONE | See [roadmap/XXII.md](roadmap/XXII.md) |

### XXIII (active)

| ID | Status | Work |
|----|--------|------|
| **XXIII-A** | DONE | Null-PC recovery |
| **XXIII-B** | DONE | RSX present every flip |
| **XXIII-C** | DONE | Audio host keep-alive |
| **XXIII-D+** | PLANNED | See [roadmap/XXIII.md](roadmap/XXIII.md) |

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
