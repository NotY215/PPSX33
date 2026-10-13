# PPSX33 Roadmap — God of War 3 (full Roman)

| Phase | Status | Focus |
|-------|--------|-------|
| I–XV | **Done** | Pipeline through fixed-function draw |
| XVI | **Done** | Audio (cellAudio HLE — waveOut 48kHz host) |
| XVII | **Done** | Pad / input (XInput + keyboard → cellPad) |
| XVIII | **Done** | PS3-native save/load only (game checkpoints / in-game save) |
| XIX | Pending | Performance (JIT hot paths, RSX batching) |
| XX | Pending | Full playable pass (title screen → in-game) |

## Save policy (port, not emulator)

- **No** savestates, **no** freeze/thaw, **no** extra save UI.
- Saves only when the **game** calls `cellSaveData*` (checkpoint / in-game save menu).
- Path: `Documents\\<GameName>\\PS3_SAVEDATA\\<dir>\\` (PARAM.SFO + game files).

## Audio (XVI)

- `cellAudioInit` / ports / start-stop HLE.
- Host: WinMM waveOut 48 kHz stereo (silence feed so ports do not stall).

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect:
```
[audio] cellAudioInit / host waveOut …
[save] root=…\\Documents\\GOW3\\PS3_SAVEDATA
[pad] XInput loaded …
```
