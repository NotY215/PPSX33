# PPSX33: PlayStation 3 Static Recompiler

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-banner.png" alt="PPSX33 neon-blue banner" width="100%" />
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.png" alt="PPSX33 circular logo" width="220" />
</p>

<p align="center"><strong>PlayStation 3 Static Recompiler</strong><br />Decode · Translate · Play</p>

PPSX33 is an experimental static recompiler for decrypted ELF64 big-endian PowerPC64 input targeting the PlayStation 3. Visit the [project website](https://notyvos.gt.tc/PPSX3/).

PPSX33 is not a complete PS3 emulator. Runtime correctness for commercial titles is not established.

## Current implementation status

| Area | Status |
| --- | --- |
| ELF / analysis / PPU lift | Implemented (static counts only — not playability) |
| Package XX-A…G | DONE |
| Runtime XX-H…O | DONE in tree (GCM, PSARC, SPU core, MFC base, SPURS base, audio, ICON0) |
| **A** RSX surfaces + blit | **DONE** |
| **B** PPU accuracy helpers | **DONE** |
| **C** SPU ISA expansion | **DONE** |
| **D** MFC / DMA fidelity | **DONE** |
| **E** SPURS jobchain | **DONE** |
| RSX full shaders/raster | Still open |
| cellFs dirs, audio ring, LV2, saves, PRX | Still open |

### Post–XX-O lettered implementations

| Sno. | Name | Status | Work |
|------|------|--------|------|
| A | RSX surfaces + color blit | DONE | Surface state, texture unit, guest color blit to D3D11 |
| B | PPU accuracy helpers | DONE | mul/div/rotate/carry, LR-first recovery |
| C | SPU ISA expansion | DONE | float, logic, rotates, compares |
| D | MFC / DMA fidelity | DONE | List EA48, tag poll, queue depth |
| E | SPURS jobchain | DONE | ready flag, chain_next, link APIs |

Details: [ROADMAP.md](ROADMAP.md).

## Documentation

- [USER_MANUAL.md](USER_MANUAL.md) — workflow, legal, troubleshooting  
- [ROADMAP.md](ROADMAP.md) — phases and lettered work  
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) — architecture  

## Build / workflow

```bat
scripts\build_all.bat
```

GUI: Load ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Successful compilation is not proof of runtime correctness.

## Links

- Website: https://notyvos.gt.tc/PPSX3/  
- [CONTRIBUTING.md](CONTRIBUTING.md) · [SUPPORT.md](SUPPORT.md) · [SECURITY.md](SECURITY.md)
