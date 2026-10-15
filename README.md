# PPSX33: PlayStation 3 Static Recompiler

<p align="center"><strong>Decode · Translate · Play</strong></p>

Experimental static recompiler for decrypted PS3 ELF64. Not a complete emulator; commercial playability is not established.

## Status

| Area | Status |
|------|--------|
| Package XX-A…G / Runtime XX-H…O | DONE in tree |
| **A–H** lettered post-O work | **DONE** (see table) |
| Full RSX shaders, deep HLE, saves, PRX | Still open |

### Lettered implementations (post XX-O)

| Sno. | Name | Status | Work |
|------|------|--------|------|
| A | RSX surfaces + blit | DONE | Surface offsets, flip tagging |
| B | PPU accuracy helpers | DONE | mul/div/rotate, LR recovery |
| C | SPU ISA expansion | DONE | float/logic/rotates |
| D | MFC / DMA fidelity | DONE | List EA48, tags, queue |
| E | SPURS jobchain | DONE | ready + chain_next |
| F | cellFs directories | DONE | opendir/readdir/closedir |
| G | Audio ring / notify | DONE | ring indices, notify key |
| H | LV2 syscalls | DONE | mutex/event/sem/thread/mem |

Runtime package for users who have not applied earlier zips: **`PPSX33_A-F_G_H.zip`** (all of A–H).

Details: [ROADMAP.md](ROADMAP.md) · End-user: [USER_MANUAL.md](USER_MANUAL.md)

## Build

```bat
scripts\build_all.bat
```

Workflow: Load ELF → Decompile → Map controls → Build → Copy to USRDIR → Run.

Website: https://notyvos.gt.tc/PPSX3/
