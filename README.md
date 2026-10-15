# PPSX33: PlayStation 3 Static Recompiler

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-banner.png" alt="PPSX33 banner" width="100%" />
</p>

<p align="center"><strong>PlayStation 3 Static Recompiler</strong><br />Decode · Translate · Play</p>

PPSX33 is an experimental static recompiler for decrypted ELF64 big-endian PowerPC64 input targeting the PlayStation 3. The repository contains a C/C++ core, command-line frontend, C# WinForms GUI, and guest runtime support. Project page: [notyvos.gt.tc/PPSX3](https://notyvos.gt.tc/PPSX3/).

PPSX33 is not a complete PS3 emulator. PPU translation, instruction semantics, system libraries, SPU execution, and RSX rendering remain separate engineering problems. Commercial-game playability is not established.

## Implementation status

| Area | Status |
|------|--------|
| ELF load / analysis / PPU static lift | Implemented (static metrics only) |
| Phases I–XIX (tooling & core pipeline) | TESTING |
| Package XX-A … XX-G | DONE |
| Runtime XX-H … XX-O | DONE in tree |
| Post-XX lettered **A – K** | DONE in tree |
| Later lettered work (L+) | PARTIAL / PLANNED — see [ROADMAP](ROADMAP.md) |

### Post-XX lettered work (A – K)

| Sno. | Name | Status |
|------|------|--------|
| A | RSX surfaces + blit | DONE |
| B | PPU accuracy helpers | DONE |
| C | SPU ISA expansion | DONE |
| D | MFC / DMA fidelity | DONE |
| E | SPURS jobchain | DONE |
| F | cellFs directories | DONE |
| G | Audio ring / notify | DONE |
| H | LV2 syscalls expand | DONE |
| I | RSX draw / shader state | DONE |
| J | PPU lifter-facing helpers | DONE |
| K | Audio multi-port | DONE |

Full phase tables and remaining gaps: [ROADMAP.md](ROADMAP.md).

## Documentation

- [USER_MANUAL.md](USER_MANUAL.md) — workflow, legal notice, troubleshooting  
- [ROADMAP.md](ROADMAP.md) — phases I–XIX, XX-A–O, lettered A–K and beyond  
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) — architecture  

## Build (Windows)

```bat
scripts\build_all.bat
```

Typical GUI flow: Load ELF → Decompile → controller map → Build → Copy to game folder → Run from `USRDIR`.

Successful compilation is not proof of runtime correctness.

## Links

- Website: https://notyvos.gt.tc/PPSX3/  
- [CONTRIBUTING.md](CONTRIBUTING.md) · [SUPPORT.md](SUPPORT.md) · [SECURITY.md](SECURITY.md) · [LEGAL.md](LEGAL.md)
