# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A | Done | UI, project-named exe, progress/ETA |
| B | Done | Halt recovery (sticky last_good, wide stack) |
| C | Done | LV2 expansion + PRX/NID filter |
| D | Done | Never hard-halt while last_good exists |
| E | **Done (on main)** | Break null-call loops + restore TOC |
| F | Next | Import table / OPD resolution (real NIDs) |
| G | Later | Real cellGcmSys state (ctrl/labels/flip) |
| H | Later | SPU + RSX command drain |

## Phase E (committed)

`runtime/ppu_runtime.h` on `main` now includes:

- Null-loop break: after 4096 hits on the same LR, try an alternate return from the stack
- Soft-break advances +4 / +8 / +16
- After 8 soft-breaks, clear sticky last_good (stop infinite spin)
- Restore r2 (TOC) from entry TOC when guest zeroes it
- Escape limit 262144; halt line includes `soft_breaks=`

## Apply (you only need git)

```bat
cd /d E:\PPSX33
git pull origin main
```

Then rebuild (GUI Build) and run GOW_3.

Look for:
```
[ps3] null-loop break: lr=0x199c78 -> alt=0x......
[ps3] null-loop break: lr=0x199c78 +4
[ps3] heartbeat ... r2=0x52d6c8
```
