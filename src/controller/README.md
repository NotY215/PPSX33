# Native C++ Controller Studio

This directory contains an independently implemented Win32 controller-mapping studio for PPSX33. It is inspired by common emulator mapping workflows, but does not copy RPCS3 source code.

## Build

On Windows with Visual Studio C++ build tools installed:

```powershell
cmake -S . -B build
cmake --build build --config Release --target PPSX33ControllerStudio
```

The executable is emitted to `build/dist/PPSX33ControllerStudio.exe`.

## Controls

- Click a mapping row, then press a keyboard key to rebind it.
- Press Escape while capturing a key to cancel.
- Click **SAVE MAP** to write `pad_map.json` next to the executable.
- The JSON uses PPSX33's existing `KeyMap` property names and retains the current default keyboard bindings.
- The UI uses a static dark theme; it does not use decorative animations.

## Integration status

This is a native C++ Win32 executable built alongside the existing application. It is not yet embedded in the existing C# WinForms Controller tab, and its save path is the executable directory rather than a selected project's `output/pad_map.json`. The existing GUI Controller tab and its settings workflow remain unchanged. A follow-up integration step is needed to host the native control in the tab and route saved mappings through the existing settings/runtime workflow.
