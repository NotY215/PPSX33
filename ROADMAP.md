# PPSX33 Roadmap

Last updated: 2026-10-11 (Phase M)

| Phase | Status | Goal |
|-------|--------|------|
| A–G | Done | Boot, GCM control/labels/FIFO, RSX D3D11 |
| H–J | Done | Outer spin detect, progress_hw, pc==lr break |
| K–L | Done | bc_taken fallthrough, early null-LR ban |
| **M** | **Done** | Mem-touch HW poke + lower thresholds (2048) |
| **N** | Next | Real NID from TOC/OPD + import table |
| O | Planned | RSX method decode / draw |
| P | Planned | SPU scheduling |

## Phase M (this turn)

Silent stall after null-loop skip to `0x199ea0` was caused by an **internal chunk loop** that never returns to `ppu_run` (so outer heartbeats never run).

Fixes:
1. **`bc_taken`**: force fallthrough after **2048** consecutive taken (was 65536); progress every 512
2. **`ps3rt_touch`**: count memory ops; every 8192 calls `progress_hw` (breaks load-based wait loops)
3. **`ppu_run`**: ban sticky null LR after 3 soft_breaks; after 6 jump to `last_good_ret` instead of stride-skipping further into dead code

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build game → Run.

Expect:
```
[ps3] Phase M force-fallthrough bc ...
[ps3] Phase M touch-progress n=...
[ps3] Phase M ban LR=0x199c78
[ps3] heartbeat ...
```
