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
| Controller tab (hide after Build) | **Done** |

## Save policy (port, not emulator)

- **No** savestates, **no** freeze/thaw, **no** extra save UI.
- Saves only when the **game** calls `cellSaveData*` (checkpoint / in-game save menu).
- Path: `Documents\\<GameName>\\PS3_SAVEDATA\\<dir>\\` (PARAM.SFO + game files).

## Audio (XVI)

- `cellAudioInit` / ports / start-stop HLE via `runtime/audio_save.cpp`.
- Host: WinMM waveOut 48 kHz stereo (silence feed so ports do not stall).

## When to copy binaries into the game folder

Use **Copy to game folder** (or copy manually) only after:

1. **Decompile** succeeds (lift report OK).
2. **Build** succeeds (MSVC exit 0, EXE under `projects/<name>/output/`).
3. A **Run from output/** test shows the window, flips, and clean exit.

Copy `<GameName>.exe` (and any needed DLLs from `output/`) next to the game's `EBOOT.BIN`.
You can keep developing from `output/` without copying. Copy to the game folder when you want the port to load USRDIR / game data like a retail install.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Decompile → Build → Run from `projects/<name>/output/`.
