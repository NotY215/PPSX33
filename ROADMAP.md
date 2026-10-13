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
| XV | **Done** | Fixed-function draw path + shader/tex method tracking (visible tint on draw) |
| XVI | Pending | Audio (cellAudio / libaudio HLE) |
| XVII | **Done** | Pad / input full mapping (XInput + keyboard → cellPad) |
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

## Phase XVII (this push)

- **XInput**: dynamic load `xinput1_4` / `1_3` / `9_1_0`; player 0 → CELL_PAD bits + sticks + L2/R2.
- **Keyboard**: WASD/arrows, K/Enter Cross, L/Esc Circle, I/Space Triangle, J/Back Square, Q/E L1/R1, Z/C L2/R2, Tab Start, Shift Select.
- **cellPad NIDs**: Init, End, GetData, GetInfo2, ClearBuf + kind-8 family stubs.
- **Poll**: every `progress_hw`; rate-limited `[pad] poll#…` log.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build (game) → Run from `projects/<name>/output/`.

Expect:
```
[pad] XInput loaded from xinput1_4.dll
[pad] poll#… btn=0x… sticks=…
[rsx] flip #… draws=… depth=… stencil=…
```

See also: [docs/games/GOW3/GOW3.md](docs/games/GOW3/GOW3.md).
