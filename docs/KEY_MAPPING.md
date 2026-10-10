# Phase: Host key mapping (deferred)

**Status: Not started — scheduled after game rendering is runnable.**

## Why later

Keyboard/gamepad mapping only matters once:
1. PPU boot reaches a stable loop,
2. RSX presents real frames (not only clear),
3. The title accepts pad input via `cellPad` / `sys_io` HLE.

Until then, key mapping cannot be tested end-to-end.

## Planned design

| PS3 control | Default host binding |
| --- | --- |
| D-Pad | Arrow keys |
| Left stick | WASD |
| Right stick | IJKL |
| Cross / Circle / Square / Triangle | Z / X / A / S (or Xbox layout) |
| L1 / R1 / L2 / R2 | Q / E / 1 / 3 |
| Start / Select | Enter / Backspace |
| L3 / R3 | F / G |

Implementation sketch:
- Host: Win32 `GetAsyncKeyState` / XInput in `ps3rt` present loop
- Guest: HLE `cellPadGetData` / `sys_hid` reads a digital/analog state buffer
- UI: optional remap dialog in C# settings (saved in settings.json)

## Acceptance

GOW3 (or homebrew pad test) reacts to host keys after a rendered frame.
