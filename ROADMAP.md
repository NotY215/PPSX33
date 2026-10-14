# PPSX33 Roadmap

Research PS3 static recompiler / HLE runtime. Not a retail port.

## Final package phase (current)

| ID | Status | Item |
|----|--------|------|
| XX-A | **Done** | Runtime package: game-named EXE + ps3rt.dll + guest_image + pad_map.json |
| XX-B | **Done** | cellFs + PSARC TOC / virtual open + zlib blocks (via zlib1.dll when present) |
| XX-C | **Done** | Copy into `PS3_GAME\USRDIR` next to EBOOT.BIN (auto-find; AppData paths) |
| XX-D | **Done** | Controller map to pad_map.json |
| XX-E | **Done** | Legal / TERMS first-run; links open in system browser |
| XX-F | **Done** | Post-build layout: reports + build_log at project root; output/ = runtime only |
| XX-G | **Done** | Named run logs: `ppsx33_log_in_game_folder_*` / `ppsx33_log_outside_game_*` |
| XX-H | **Pending** | Full playable pass (title → in-game UI / audio / assets) |

## Packaging notes (current)

- **Output EXE name** = project/game folder name (e.g. `God of War III.exe`), not always `PPSX33.exe`.
- **Copy to game folder** destination is always the directory that contains `EBOOT.BIN` (typically `PS3_GAME\USRDIR`).
- **Pad / XInput log** is written only while the game window is focused.
- **Boot** runs `ps3rt_assets_boot_scan()` after guest image map (PSARC/cellFs readiness).
- **Window/taskbar icon** can use `PS3_GAME\ICON0.PNG` when present (host `WM_SETICON`).

## Earlier phases (I–XIX)

| Phase | Status | Focus |
|-------|--------|-------|
| I | **Done** | UI, project pipeline, progress/ETA |
| II | **Done** | Halt recovery (sticky last_good, wide stack walk) |
| III | **Done** | LV2 expansion + PRX/NID filter |
| IV | **Done** | Never hard-halt while last_good exists |
| V | **Done** | Break null-call loops + restore TOC + real GCM state |
| VI | **Done** | Import table / OPD resolution + cellGcmSys stub expansion |
| VII | **Done** | SPU bring-up (mailbox HLE, auto-complete) |
| VIII | **Done** | RSX command-buffer drain + FIFO method decode |
| IX | **Done** | D3D11/D3D10 host backend + Present |
| X | **Done** | Region-ban walk guards + high last_good preference |
| XI | **Done** | Timestamped run log next to game EXE |
| XII | **Done** | Honour real guest GCM put; inject only when FIFO stalled |
| XIII | **Done** | Surfaces / wild-PC recovery / GCM nid=0 / LV2 event queue |
| XIV | **Done** | Depth / stencil / clear surface fidelity |
| XV | **Done** | Fixed-function draw path + shader/tex method tracking |
| XVI | **Done** | Audio (cellAudio / libaudio HLE — waveOut 48 kHz host) |
| XVII | **Done** | Pad / input full mapping (XInput + keyboard → cellPad) |
| XVIII | **Done** | PS3-native save/load only (no savestates) |
| XIX | **Done** | Performance: RSX FIFO batch, coalesced Present |

## Project folder after successful Build

```
projects/<GameName>/
  analysis_report.txt
  lift_report.txt
  build_log.txt
  output/
    <GameName>.exe
    ps3rt.dll
    guest_image.bin
    pad_map.json
```

`input/` and `codebase/` are removed after Build succeeds.

## Settings (`%APPDATA%/PPSX33/settings.json`)

- Rpcs3Path
- LastElfPath / LastBinPath
- LastGameFolderPath / LastGamesRootPath / LastEbootPath
- LastOutputFolderPath
- Graphics, threads, key map, terms flags

## SPU status (research)

- Implemented: mailbox HLE / auto-complete, basic `SPUContext` array.
- Not implemented: full SPU ISA interpreter/JIT, vector/FP ops, MFC DMA, SPURS, stop/exception model.
- App shows a permanent launch notice. Report missing SPU behaviour with the **SPU coverage** issue template and attach the latest run log.

## Why UI / assets / sound may be missing (XX-H)

A solid blue window with only GCM null-import stubs, null-loop soft_breaks, and PC=0 halt means the guest never reached cellFs / cellAudio / real draw submission. Deeper PRX/OPD fidelity and control-flow past the null-loop are required — tracked under **XX-H**.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Decompile → Build → Copy to game folder → run the game-named EXE from `USRDIR`.
