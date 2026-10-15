# PPSX33: PlayStation 3 Static Recompiler

Experimental static recompiler for **decrypted** PS3 ELF64. Not a complete emulator. Commercial playability is not established.

## Status

| Band | Status |
|------|--------|
| PHASE 1 — I–XXI | TESTING / DONE (see [ROADMAP.md](ROADMAP.md)) |
| **PHASE XXII** | **Active** — pipeline correctness |
| OpenGL fallback | Future |

## Requirements

### Required

| Item | Spec |
|------|------|
| **Disk type** | HDD or SSD (SSD preferred for game data and builds) |
| **RAM** | 8 GB system memory |
| **GPU** | Any GPU with a working WDDM driver (integrated graphics OK) |
| **VRAM** | Shared system memory or dedicated; enough for a D3D11 swap chain (roughly 512 MB-class integrated is enough for clear/present) |
| **Monitor** | 1280×720 or higher |
| **Graphics API** | Direct3D 11, Feature Level 10_0 or higher (current path) |
| **OS** | Windows 10 or later, 64-bit |
| **Content** | Decrypted ELF/EBOOT you legally own; encrypted SELF is not loaded |

### Recommended

| Item | Spec |
|------|------|
| **Disk type** | SSD or NVMe |
| **RAM** | 16 GB or more |
| **GPU** | Discrete GPU with Direct3D 11 Feature Level 11_0 |
| **VRAM** | 2 GB or more dedicated |
| **Monitor** | 1920×1080 |
| **Graphics API** | D3D11 FL 11_0; future auto-fallback to OpenGL 3.3 (or 2.1) if D3D11 is unsuitable |

These are **host tool** requirements. They do not imply that any game will run correctly.

## Documentation

- [ROADMAP.md](ROADMAP.md) — PHASE 1–XXII index  
- [roadmap/](roadmap/) — per-phase pages (I.md, XX-A.md, XXI.md, XXII.md, …)  
- [USER_MANUAL.md](USER_MANUAL.md)  

## Build

```bat
scripts\build_all.bat
```

Workflow: Load decrypted ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
