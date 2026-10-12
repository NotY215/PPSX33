# PPSX33 Roadmap — God of War 3 (full Roman)

| Phase | Status | Focus |
|-------|--------|-------|
| I | Done | UI, project-named EXE, progress/ETA, basic project pipeline |
| II | Done | Halt recovery (sticky last_good, wide stack walk) |
| III | Done | LV2 expansion + PRX/NID filter |
| IV | Done | Never hard-halt while last_good exists |
| V | Done | Break null-call loops (soft_breaks / stride) + restore TOC + real GCM state |
| VI | Done | Import table / OPD resolution + cellGcmSys stub expansion |
| VII | Done | SPU bring-up (mailbox HLE, auto-complete) |
| VIII | Done | RSX command-buffer drain + FIFO method decode |
| IX | Done | D3D11/D3D10 host backend + Present |
| X | Done | Region-ban walk guards + high last_good preference |
| XI | Done | Timestamped run log next to game EXE (`log_YYYY-MM-DD_HH-MM-SS.txt`) |
| XII | Done | Honour real guest GCM put; inject only when FIFO stalled |
| XIII | Pending | Textures / surface management / real draws |
| XIV | Pending | Depth / stencil / clear surface fidelity |
| XV | Pending | Shader translation (VP/FP) or fixed-function path for GOW3 |
| XVI | Pending | Audio (cellAudio / libaudio HLE) |
| XVII | Pending | Pad / input full mapping |
| XVIII | Pending | Save data / trophy stubs |
| XIX | Pending | Performance (JIT hot paths, RSX batching) |
| XX | Pending | Full playable pass (title screen → in-game) |

## Phase XII (this push)

- `ps3rt_progress_hw` tracks guest put at GCM control+0.
- When put advances → drain FIFO via `ps3rt_rsx_set_put` + `flush` (no synthetic spam).
- Synthetic CLEAR/viewport inject only if put stalls for >64 ticks.
- cellGcmSys stub inject rate reduced (every 64th call).
- Log line: `[gcm] Phase XII guest put 0x.. -> 0x..`

## Critical restore

- **project.cpp** restored (full `lift_project` / `build_project`) — Decompile works again.
- **ps3rt.cpp** restored (was placeholder on main) with log tee + Phase XII.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Decompile → Build → Run.

Expect on Decompile: real analysis (segments, OPDs, chunks) — **not** the restore message.
Expect on Run: `[ps3rt] log file: ...` and optionally `[gcm] Phase XII guest put ...`.
