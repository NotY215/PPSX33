# PPSX33: PlayStation 3 Static Recompiler

<p align="center"><strong>Decode · Translate · Play</strong></p>

Experimental static recompiler for decrypted PS3 ELF64. Not a complete emulator.

## Status

| Area | Status |
|------|--------|
| I–XIX / XX-A…O | TESTING / DONE |
| Post-XX lettered **A – Q** | DONE in tree |
| Later (R+) | PARTIAL / PLANNED |

### Lettered A – Q (summary)

Surfaces through texture unit (A–L), PPU/SPU/MFC/SPURS (B–E), filesystem/audio/LV2 (F–K, N), library NID stubs (**O**), save lifecycle (**P**), pad pressure/sixaxis (**Q**).

Full tables: [ROADMAP.md](ROADMAP.md).

## Build

```bat
scripts\build_all.bat
```

Load decrypted ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
