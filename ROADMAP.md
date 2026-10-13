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
| XVI | **Done** | Audio (cellAudio / libaudio HLE — waveOut 48 kHz host) |
| XVII | **Done** | Pad / input full mapping (XInput + keyboard → cellPad) |
| XVIII | **Done** | PS3-native save/load only (game checkpoints / in-game save; no savestates) |
| XIX | **Done** | Performance: RSX FIFO batch (16k steps), coalesced Present, progress batch + flush |
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
| Controller tab (hide after Build) | **Done** |

## Phase XIX notes

- FIFO drain budget raised 4096 → 16384 methods per pass.
- Present skipped on empty frames (no draws); forced every 8th tick to keep window alive.
- `progress_hw` batches every 128 ticks: guest FIFO flush + one flip.

## Save policy (port, not emulator)

- **No** savestates. Saves only when the game calls `cellSaveData*`.
- Path: `Documents\\<GameName>\\PS3_SAVEDATA\\<dir>\\`

## When to copy binaries into the game folder

After Decompile + Build succeed and Run-from-`output/` looks good: use **Copy to game folder** (EXE next to `EBOOT.BIN`).

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Decompile → Build → Run from `projects/<name>/output/`.

## Current GUI workflow behavior

The WinForms GUI follows **Load ELF / Decrypt EBOOT → Decompile → Build → Run**, with output copying available after a successful build. The current UI also provides controller-key configuration before build, graphics-backend and PPU-thread options, Console/Report/Roadmap tabs, and an object-file-based build progress/ETA estimate.

- **Progress/ETA:** inferred from `.obj` file changes; estimates are advisory and can be inaccurate for incremental builds, linking, or compiler scheduling.
- **Status messages:** operation-specific success, failure, and stop states are intended to remain distinct.
- **Stop Game:** terminates the active game process tree.
- **Cancel Build:** asks for confirmation, calls the native cancellation API, and then clears the selected project and removes generated `input/`, `codebase/`, and `output/` folders. The ELF must be loaded again to continue.
- **Decompile:** does not currently expose cooperative cancellation.
- **Cleanup:** a successful build preserves output binaries and report copies; the GUI and native build driver perform different cleanup steps, so test cancellation and successful-build cleanup on disposable projects before relying on their effects.

These are code-path descriptions, not a claim that every cancellation race or toolchain configuration has been runtime-tested. See [the developer guide](docs/DEVELOPER_GUIDE.md) for implementation details.
