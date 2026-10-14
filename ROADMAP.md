# PPSX33 Roadmap

Research PS3 static recompiler / HLE runtime. Not a retail port.

## Final package phase (current)

| ID | Status | Item |
|----|--------|------|
| XX-A | **Done** | Runtime package: EXE + ps3rt.dll + guest_image |
| XX-B | **Done** | cellFs + PSARC TOC / virtual open + zlib blocks (via zlib1.dll when present) |
| XX-C | **Done** | Copy to game folder (auto-find EBOOT.BIN; paths in AppData) |
| XX-D | **Done** | Controller map to pad_map.json |
| XX-E | **Done** | Legal / TERMS first-run; links open in system browser |
| XX-F | **Done** | Post-build layout: reports + build_log at project root; output/ = runtime only; no input/codebase |
| XX-G | **Done** | Named run logs: ppsx33_log_in_game_folder_* / ppsx33_log_outside_game_* |
| XX-H | Pending | Full playable pass (title to in-game UI / audio / assets) |

## Earlier phases (I-XIX)

Core recompiler, HLE stubs, GUI workflow, MSVC pipeline, RSX D3D11 present, pad, audio HLE, save HLE — **Done**.

## Project folder after successful Build

```
projects/<GameName>/
  analysis_report.txt
  lift_report.txt
  build_log.txt
  output/
    PPSX33.exe
    ps3rt.dll
    guest_image.bin
    pad_map.json
```

`input/` and `codebase/` are removed after Build succeeds.

## Settings (%APPDATA%/PPSX33/settings.json)

- Rpcs3Path
- LastElfPath / LastBinPath
- LastGameFolderPath / LastGamesRootPath / LastEbootPath
- LastOutputFolderPath
- Graphics, threads, key map, terms flags

**Copy to game folder:** user selects a game dump folder; PPSX33 finds EBOOT.BIN automatically and stores the game folder + parent games root (e.g. E:/Games).

## Why UI / assets / sound may be missing

Logs that show only GCM null-import stubs, null-loop soft_breaks, and PC=0 halt mean the guest never reached cellFs / cellAudio / real draw submission. Fixing that requires deeper PRX/OPD fidelity — tracked under XX-H.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All then Decompile then Build then Run or Copy to game folder.
