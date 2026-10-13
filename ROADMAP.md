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
| XVI | **Next** | Audio (cellAudio / libaudio HLE) |
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

## Phase XV (this push)

- **Fixed-function draw**: when FIFO draw methods fire, host presents a tinted overlay (proves draw path is live).
- **Shader / texture method tracking**: VP/FP bind and tex methods counted (`shaders=` / `tex=` in log).
- **Viewport / scissor** method accept.
- **Depth/stencil inject**: CLEAR_SURFACE + depth/stencil values so `depth=` / `stencil=` rise.
- Real VP/FP ISA translation still deferred to a later pass under XV/XIX.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build (game) → Run from `projects/<name>/output/`.

Expect:
```
[rsx] D3D11 host present ready … (depth+stencil)
[rsx] flip #… draws=… clears=… depth=… stencil=… shaders=… tex=…
```

Window: dark slate with shifting warm tint when draws run. Close window for clean exit.

See also: [docs/games/GOW3/GOW3.md](docs/games/GOW3/GOW3.md).
