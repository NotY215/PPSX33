# PPSX33: PlayStation 3 Static Recompiler

<p align="center">
  <img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-banner.png" alt="PPSX33 banner" width="100%" />
</p>

<p align="center"><strong>PlayStation 3 Static Recompiler</strong><br />Decode · Translate · Play</p>

Experimental static recompiler for decrypted PS3 ELF64. Not a complete emulator; commercial playability is not established.

## Status

| Area | Status |
|------|--------|
| Phases I–XIX | TESTING |
| Package XX-A … XX-G | DONE |
| Runtime XX-H … XX-O | DONE in tree |
| Post-XX lettered **A – X** | DONE in tree |
| Optional Y+ | PLANNED |

### Post-XX lettered (summary)

| Range | Focus |
|-------|--------|
| **A–L** | RSX surfaces/texture unit, PPU helpers, SPU/MFC/SPURS |
| **F–K, N** | cellFs, audio, LV2, errno/SELF |
| **O–Q** | NID stubs, save lifecycle, pad pressure |
| **R–U** | Timing, PRX registry, textured draws, sysutil params |
| **V** | Host texture upload from guest |
| **W** | PRX relocate HLE (base/entry/exports) |
| **X** | Frame pacing (~60 Hz) |

Full tables: [ROADMAP.md](ROADMAP.md).

## Documentation

- [USER_MANUAL.md](USER_MANUAL.md)  
- [ROADMAP.md](ROADMAP.md)  
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)  

## Build

```bat
scripts\build_all.bat
```

Load decrypted ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
