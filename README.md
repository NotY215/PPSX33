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
| Post-XX lettered **A – N** | DONE in tree |
| Later work (O+) | PARTIAL / PLANNED |

### Post-XX lettered (A – N)

| Sno. | Name | Status |
|------|------|--------|
| A–K | Surfaces through multi-port audio / LV2 | DONE |
| L | RSX texture unit tracking | DONE |
| M | PPU lifter helper surface | DONE |
| N | cellFs errno + SELF rejection | DONE |

Details: [ROADMAP.md](ROADMAP.md).

## Documentation

- [USER_MANUAL.md](USER_MANUAL.md)  
- [ROADMAP.md](ROADMAP.md)  
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)  

## Build

```bat
scripts\build_all.bat
```

Workflow: Load decrypted ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
