# Key Mapping

PPSX33 uses the following default keyboard bindings for the PlayStation 3 DualShock-style controls. These defaults are defined in `src/gui/KeyMap.cs` and are stored in the `KeyMap` section of `%APPDATA%/PPSX33/settings.json`.

## Default bindings

| PS3 control | Default keyboard binding |
| --- | --- |
| Left Stick Left | Left Arrow |
| Left Stick Down | Down Arrow |
| Left Stick Right | Right Arrow |
| Left Stick Up | Up Arrow |
| Right Stick Left | H |
| Right Stick Down | J |
| Right Stick Right | K |
| Right Stick Up | U |
| Start | V |
| Select | Space |
| PS Button | Backspace |
| Square | A |
| Cross | Z |
| Circle | X |
| Triangle | S |
| D-Pad Left | Numpad 4 |
| D-Pad Down | Numpad 5 |
| D-Pad Right | Numpad 6 |
| D-Pad Up | Numpad 8 |
| R1 | R |
| R2 | Right Ctrl |
| R3 | Left Ctrl |
| L1 | L |
| L2 | Right Shift |
| L3 | Left Shift |

## Settings persistence

The GUI settings model stores these values in `%APPDATA%/PPSX33/settings.json` under `KeyMap`. If no settings file exists, PPSX33 creates its settings model with these defaults. Existing saved values are retained when settings are loaded.

The internal key names used by the C# model include `NumPad4`, `RControlKey`, `LControlKey`, `RShiftKey`, and `LShiftKey`; the table above uses user-facing names.

## Implementation status

The default bindings and settings model are present in the GUI code. End-to-end input depends on the runtime polling host keyboard state and passing the resulting button and analog-stick state to the guest pad HLE. Treat game input as incomplete until this path is connected and verified with a homebrew pad test or a supported game. A configured mapping by itself does not prove that guest input is working.

## Related source

- `src/gui/KeyMap.cs`: default binding values.
- `src/gui/Settings.cs`: loading and saving `settings.json`.
- `runtime/ps3rt.cpp`: guest pad-state setter, `ps3rt_pad_set`.
