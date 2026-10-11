# Phase F (2026-10-11)

## Changes (applied in runtime/)

### ppu_runtime.h
- Treat a 0x100 PC window as the same null-call "site" (table walks)
- Break after 512 hits (was 4096)
- Stride grows: 0x40 -> 0x100 -> 0x400 -> 0x1000 to skip whole tables
- Clear sticky after 48 soft-breaks

### ps3rt.cpp
- try_resolve_opd(): read PowerPC OPD (entry@0, toc@8) from guest pointers
- Soft-success when r11/ctr point at data-segment OPD slots (GOW3 unresolved imports)

## Apply
```
git pull origin main
```
Rebuild and run GOW_3. Look for:
```
[ps3] null-loop skip: 0x33723c +0x40 -> 0x33727c
[prx] OPD@0x...... -> entry=0x...... (soft)
```
