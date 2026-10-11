# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A | Done | UI, project-named exe, progress/ETA |
| B | Done | Halt recovery (sticky last_good, wide stack) |
| C | Done | LV2 expansion + PRX/NID filter |
| D | Done | Never hard-halt while last_good exists |
| E | Done | Break null-call loops + restore TOC |
| F | **Done (build fix)** | Null-table skip + OPD soft-resolve + header copy |
| G | Next | Real cellGcmSys state (ctrl/labels/flip) |
| H | Later | SPU + RSX command drain |

## Phase F (this turn)

- Aggressive null-table skip (stride 0x40..0x1000)
- Split headers: `ppu_runtime_core_*.h` + `ppu_runtime_run.h`
- `project.cpp` now copies split headers into game `output/src/`

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Then **Rebuild All** (so ps3core.dll gets the new project.cpp), then **Build** the game.

Look for:
```
[ps3] null-loop skip: 0x33723c +0x40 -> 0x33727c
[ps3] heartbeat ... r2=0x52d6c8
```
