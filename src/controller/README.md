# Native C++ Controller Studio

PPSX33's controller mapping interface is an independently implemented native Win32 child window written in C++. It is embedded inside the existing WinForms **Controller** tab. It does not copy RPCS3 source code and does not launch a separate controller executable.

## Build

On Windows with Visual Studio C++ build tools, CMake, and the .NET 8 SDK installed:

```powershell
cmake -S . -B build
cmake --build build --config Release --target PS3Recomp
```

The main UI remains `build/dist/PS3Recomp.exe`. The native controller implementation is a companion library, `build/dist/PPSX33ControllerNative.dll`; no `PPSX33ControllerStudio.exe` is generated. The existing CLI target is separate from the GUI and is not part of the controller implementation.

## Mapping behavior

- Click a binding row, then press a keyboard key to rebind it.
- Press Escape while capturing a key to cancel.
- Click **SAVE MAP** to save the map through the existing PPSX33 settings workflow.
- Saved mappings are persisted to `%APPDATA%/PPSX33/settings.json` and, when a project is loaded, to that project's `output/pad_map.json`.
- The JSON uses the existing `KeyMap` property names and current default keyboard bindings.
- The interface uses a static dark theme without decorative animations.
- The native window is hosted in the existing UI; no second controller process is created.

## Architecture

`ControllerStudio.cpp` exports a small C ABI for creating, resizing, updating, and destroying the child window. `MainForm.Controller.cs` hosts that child window and routes its saved JSON mapping to PPSX33's existing C# settings and project-output code. CMake builds the controller component as a DLL and makes the main GUI target depend on it.
