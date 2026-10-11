# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A–F | Done | UI → null-table skip |
| G | Done | Real cellGcmSys state (ctrl/labels/flip) |
| **H** | **Done** | Spin-loop detection + `ps3rt_progress_hw` |
| I | Next | Deeper RSX method decode / real draws |

## Phase H

- `ppu_runtime_run.h`: same-PC spin detector → calls `ps3rt_progress_hw()` every 4096 hits
- `ps3rt.cpp`: `ps3rt_progress_hw()` drains GCM put→get, auto-flips, clears labels
- After long spins, tries alternate return from stack

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build game.

### Look for
```
[ps3] Phase H spin pc=0x32eb24 count=... pokes=...
[rsx] flip #...
[ps3] Phase H break spin via alt=0x...
```
