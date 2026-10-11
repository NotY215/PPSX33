# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A | Done | UI, project-named exe, progress/ETA |
| B | Done | Halt recovery (sticky last_good, wide stack) |
| C | Done | LV2 expansion + PRX/NID filter |
| D | Done | Never hard-halt while last_good exists |
| E | Done | Break null-call loops + restore TOC |
| F | **In progress** | Import/OPD resolution + aggressive table skip |
| G | Later | Real cellGcmSys state (ctrl/labels/flip) |
| H | Later | SPU + RSX command drain |

## Apply
```bat
cd /d E:\PPSX33
git pull origin main
```
Then rebuild (GUI Build) and run GOW_3.
