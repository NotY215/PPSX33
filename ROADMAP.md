# PPSX33 Roadmap — God of War 3 (full Roman)

| Phase | Status | Focus |
|-------|--------|-------|
| I | **Done** | UI, project-named EXE, progress/ETA, basic project pipeline |
| II | **Done** | Halt recovery (sticky last_good, wide stack walk) |
| III | **Done** | LV2 expansion + PRX/NID filter |
| IV | **Done** | Never hard-halt while last_good exists |
| V | **Done** | Break null-call loops (soft_breaks / stride) + restore TOC + real GCM state |
| VI | **Done** | Import table / OPD resolution + cellGcmSys stub expansion |
| VII | **Done** | SPU bring-up (mailbox HLE, auto-complete) |
| VIII | **Done** | RSX command-buffer drain + FIFO method decode |
| IX | **Done** | D3D11/D3D10 host backend + Present |
| X | **Done** | Region-ban walk guards + high last_good preference |
| XI | **Done** | Timestamped run log next to game EXE (`log_YYYY-MM-DD_HH-MM-SS.txt`) |
| XII | **Done** | Honour real guest GCM put; inject only when FIFO stalled |
| XIII | **Done** | Surfaces / wild-PC recovery / GCM nid=0 useful returns / LV2 event queue |
| XIV | **Done** | Depth / stencil / clear surface fidelity (DSV, mask, no pure-blue clear) |
| XV | **Next** | Shader translation (VP/FP) or fixed-function path for GOW3 |
| XVI | Pending | Audio (cellAudio / libaudio HLE) |
| XVII | Pending | Pad / input full mapping |
| XVIII | Pending | Save data / trophy stubs |
| XIX | Pending | Performance (JIT hot paths, RSX batching) |
| XX | Pending | Full playable pass (title screen → in-game) |

## Parallel track (tooling)

| Item | Status |
|------|--------|
| BUILD_TOTAL + live console + ETA | **Done** |
| MSVC rsp link files (no 8191 limit) | **Done** |
| PATH shrink before vcvars | **Done** |
| Post-build cleanup (codebase/ + input/) | **Done** |
| GUI cancel unlock after first cancel | **Done** |
| Clean window close (force_exit) | **Done** |

## Phase XIV (current)

- **Clear colour**: cycle dark slate (`0x101828`…`0x283848`) — removed pure-blue `0xF1` inject.
- **Depth/stencil**: D24S8 DSV, clear depth+stencil each present; resize recreates DSV.
- **Clear surface mask**: `0x01D4` / small `0x01D8` args treated as color/depth/stencil bits.
- **Draw inject**: `0x1800` so log `draws=` rises with clears.
- **Log line**: `flip #N draws=… clears=… depth=… stencil=… methods=…`

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build (game) → Run from `projects/<name>/output/`.

Expect:
```
[stage] BOOT … MEM … RSX … GCM … PPU … RUNNING
[rsx] D3D11 host present ready … (depth+stencil)
[rsx] flip #… draws=… clears=… depth=… stencil=… known=…
```

Window should be **dark slate** (not pure blue). Close the window for a clean exit.

See also: [docs/games/GOW3/GOW3.md](docs/games/GOW3/GOW3.md).
