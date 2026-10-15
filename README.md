# PPSX33: PlayStation 3 Static Recompiler

<p align="center"><strong>Decode · Translate · Play</strong></p>

Experimental static recompiler for decrypted PS3 ELF64. Not a complete emulator.

## Status

| Area | Status |
|------|--------|
| Phases I–XIX | TESTING |
| Package XX-A … XX-G | DONE |
| Runtime XX-H … XX-O | DONE |
| **Phase XXI (lettered A – Z)** | DONE in tree |

**Phase XXI** is the name for all post–XX-O lettered work **A through Z** (surfaces, SPU/MFC/SPURS, cellFs, audio, LV2, NID/PRX, RSX texture upload/blit, frame pacing, and more).

Full tables: [ROADMAP.md](ROADMAP.md).

## Requirements

Host minimum and recommended specifications are documented in [requirements.md](requirements.md). These are host-tool targets, not a promise that a specific game will run correctly.

## Build

```bat
scripts\build_all.bat
```

Load decrypted ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
