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
| XIII | Done | Surfaces / wild-PC recovery / GCM nid=0 useful returns / LV2 event queue |
| XIV | Pending | Depth / stencil / clear surface fidelity |
| XV | Pending | Shader translation (VP/FP) or fixed-function path for GOW3 |
| XVI | Pending | Audio (cellAudio / libaudio HLE) |
| XVII | Pending | Pad / input full mapping |
| XVIII | Pending | Save data / trophy stubs |
| XIX | Pending | Performance (JIT hot paths, RSX batching) |
| XX | Pending | Full playable pass (title screen → in-game) |

## Phase XIII (this push)

- **Wild-PC guard**: PC == 0 or PC >= 0x10000000 snaps back via stack/last_good (stops `0xfffffffffe…` walks).
- **GCM nid=0**: unknown cellGcmSys calls still return control / labels / flip-status in rotation.
- **LV2 sc 128**: event-queue create writes id to r3 or r4; more event-queue ops as success.
- **RSX**: varying clear colour, more frequent flip logs, texture-offset tracking.
- All run-loop logs go through `ps3rt_log` (file + stderr).

## Apply

```bat
cd /d E:\\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect:
```
[gcm] Phase XII guest put …
[ps3] Phase XIII wild-pc 0x… -> 0x…
[rsx] flip #… draws=… clears=… known=… unknown=0
```
