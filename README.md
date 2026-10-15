# PPSX33: PlayStation 3 Static Recompiler

Experimental static recompiler for decrypted PS3 ELF64. Not a complete emulator.

## Status

| Area | Status |
|------|--------|
| I–XIX / XX-A…O | TESTING / DONE |
| Post-XX lettered **A – U** | DONE in tree |
| Optional V+ | PLANNED |

### Latest letters

| Sno. | Name | Status |
|------|------|--------|
| R | Timing model | DONE |
| S | PRX module registry | DONE |
| T | RSX textured-draw awareness | DONE |
| U | Deeper sysutil NID behaviour | DONE |

Full history: [ROADMAP.md](ROADMAP.md).

## Build

```bat
scripts\build_all.bat
```

Load decrypted ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
