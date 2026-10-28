# PPSX33 User Manual

**PlayStation 3 Static Recompiler — Decode · Translate · Validate**

This manual explains how to use the PPSX33 application safely and effectively.  
PPSX33 is an experimental research toolkit. It is **not** a commercial emulator, a finished product, or a way to play pirated games.

---

## 1. Legal notice (read before use)

### 1.1 Ownership and copyright

- PlayStation, PlayStation 3, PS3, CELL, RSX, and related marks are trademarks of Sony Interactive Entertainment Inc.
- All game software, assets, icons (ICON0.PNG, etc.), and related materials remain the property of their respective copyright holders.
- PPSX33 does not intentionally bundle commercial game code, game assets, or proprietary keys. Review release contents and third-party asset licenses before redistribution.
- PPSX33 loads **decrypted ELF** files only. It does not decrypt EBOOTs or launch external emulators. Obtaining a decrypted ELF is the user’s responsibility.

### 1.2 What you may and may not do

**You may:**

- Use PPSX33 only with files and data you are authorized to access and process under applicable law and relevant license/platform terms. Ownership of a disc or copy does not automatically answer every question about decryption, circumvention, or derivative output.
- Study the recompiler, contribute improvements, and report bugs.

**You may not:**

- Use PPSX33 to play or distribute pirated, cracked, or illegally obtained games.
- Redistribute game binaries, assets, or decryption keys.
- Claim that PPSX33 is an official Sony product or that it guarantees compatibility with any commercial title.

### 1.3 First-run TERMS dialog

On first launch the application shows a TERMS / legal notice. You must accept it to continue. Links open in your system browser (not inside the app). Re-reading the TERMS is always available from the UI.

### 1.4 Disclaimer

PPSX33 is provided **as-is**, without warranty of any kind. Runtime behaviour is experimental. Commercial games may hang, show a solid blue screen, crash, or produce incorrect audio/video. Use at your own risk. The authors are not responsible for data loss, system instability, or legal consequences arising from misuse.

---

## 2. System requirements

| Item | Requirement |
|------|-------------|
| OS | Windows 10 / 11 (64-bit) |
| Visual C++ | MSVC toolchain (Visual Studio 2019 or newer recommended) for builds |
| .NET | Compatible .NET runtime for the WinForms GUI |
| GPU / display | Direct3D 11 capable GPU for the host present path |
| Optional | A decrypted ELF (how you obtain it is outside this app) |
| Optional | `zlib1.dll` next to the game executable (required for PSARC streaming) |
| Optional | XInput-compatible controller |

---

## 3. Getting started — quick workflow

1. **Launch** `PPSX33` (the GUI).
2. Accept the **TERMS** dialog if shown.
3. **Load** a decrypted ELF you are authorized to process. Encrypted SELF is not supported inside PPSX33.
4. **Decompile** (lift) the ELF — this produces translated chunks.
5. Optionally set **controller mappings** (must be done before Build).
6. Click **Build**. Wait until the status shows success.
7. Click **Copy to game folder**. This places the game-named `.exe`, `ps3rt.dll`, and related files next to the original EBOOT in `PS3_GAME\USRDIR` (or the folder you chose).
8. Click **Run** (or start the generated `.exe` from the USRDIR folder).

A successful build does **not** guarantee the game will play correctly. Runtime support is still experimental.

---

## 4. Main window overview

| Control | Purpose |
|---------|---------|
| **Load ELF** | Select a decrypted ELF / EBOOT file and create a project. |
| **Decompile** | Run the PPU static lifter (chunk translation). |
| **Build** | Compile the lifted code + runtime into a game-named executable and `ps3rt.dll`. |
| **Copy to game folder** | Copy the built output into the game’s `USRDIR` (next to EBOOT.BIN). Auto-discovers EBOOT when possible. |
| **Run** | Launch the generated executable. |
| **Stop / Cancel** | Stops a running game, or requests cancellation of an in-progress build. Cancelling a build does **not** delete your whole project folder. |
| **Controller / pad map** | Map keyboard or XInput buttons to PS3 pad buttons. Saved as `pad_map.json`. |
| **Graphics / threads** | Host present and thread options (unlocked after successful lift/build stages). |
| **Log / Report / Roadmap tabs** | Live log, analysis report, and in-app roadmap status. |

During a build most action buttons are locked so you cannot start conflicting operations.

---

## 5. Step-by-step guide

### 5.1 Preparing a game

1. Obtain a **legally owned** copy of the game.
2. Use a decrypted ELF only when you are authorized to access and process it.
3. Keep the original `PS3_GAME` folder structure if you want ICON0, PSARC assets, and USRDIR layout to work correctly.

### 5.2 Loading and lifting

1. Click **Load ELF** and select the decrypted file, **or**
2. Click **Load ELF** and select a decrypted ELF.
3. Click **Decompile**. Progress and ETA appear in the status area. Large titles can take a long time.
4. When finished, the Report tab and log show lift statistics.

### 5.3 Controller mapping

1. Open the controller / pad map UI **before** building.
2. Assign keyboard keys or XInput buttons to the classic PS3 buttons (Cross, Circle, Square, Triangle, D-pad, sticks, L1/R1/L2/R2, Start, Select, etc.).
3. Save. The mapping is written to `pad_map.json` and used at runtime.
4. Pad input is logged only while the game window has focus (to reduce noise).

### 5.4 Building

1. Click **Build**.
2. The MSVC project is generated and compiled. Status and log show progress.
3. On success you get:
   - A game-named `.exe`
   - `ps3rt.dll`
   - `guest_image.bin` (and reports) under the project `output\` folder
4. If the build fails, read the log for MSVC errors (Cxxxx / LNKxxxx). Common fixes are already documented in the repository issue templates.

### 5.5 Copying to the game folder

1. Click **Copy to game folder**.
2. The tool tries to locate `EBOOT.BIN` / `USRDIR` automatically.
3. Built files are placed next to the EBOOT so the game can find assets, ICON0, and PSARC packages relative to the executable.

### 5.6 Running

1. Click **Run**, or double-click the generated `.exe` inside `USRDIR`.
2. A window opens. Title, status overlay, and (if present) ICON0.PNG are applied when possible.
3. Audio uses the host waveOut path when the game calls cellAudio.
4. Use **Stop** to end the process cleanly.

---

## 6. Runtime features (what works today)

These items are implemented in the current tree (XX-H … XX-O):

| Feature | Notes |
|---------|--------|
| Null-loop escape | Helps the PPU leave tight empty loops that previously hung the recompiler. |
| cellGcmSys HLE | Display buffers, flip/wait, map, tiles — basic present path. |
| PSARC + zlib | TOC parsing and block streaming; requires `zlib1.dll` beside the exe. |
| SPU interpreter | Subset of SPU ISA (loads/stores, arithmetic, branches, channels). |
| MFC / DMA / tags | List DMA, barriers, atomics, tag waits. |
| SPURS-like jobs | Job queue + cellSpurs HLE across 8 SPU slots. |
| cellAudio | Port / AddData → host waveOut playback. |
| Status overlay | GDI on-screen status text. |
| ICON0 icon | Window and taskbar icon loaded from `ICON0.PNG` / `icon0.png` near the game. |

**Still experimental / incomplete**

- Full RSX drawing (many titles still show solid blue until real draws occur).
- Complete SPU ISA and accurate SPURS scheduling.
- Full system call / library coverage.
- Perfect audio timing and save-data behaviour.

---

## 7. Settings and persistence

- Project directory, last ELF, graphics/thread choices, and UI state are saved between sessions.
- Controller map is stored as `pad_map.json`.
- Named run logs are written so you can attach them to bug reports.
- Cancelled builds no longer wipe the entire project folder.

---

## 8. Troubleshooting

| Symptom | What to try |
|---------|-------------|
| Solid blue screen | Expected for many titles until RSX draws and assets load. Check log for GCM / flip activity. |
| Immediate hang / black window | Attach the latest run log. Null-loop escape and GCM HLE may still need more work for that title. |
| “Missing zlib1.dll” | Place a 64-bit `zlib1.dll` next to the game executable. |
| Build C4129 (escape sequence) | Ensure `runtime/rsx_stub.cpp` uses forward slashes for ICON0 paths (see build-fixes package). |
| Build C2732 (linkage) | Ensure `ps3rt_spu_interpreter_start` is declared and defined with `extern "C"`. |
| No sound | Confirm cellAudio is being called; check waveOut device availability. |
| No icon | Place `ICON0.PNG` (or `icon0.png`) in `PS3_GAME` or next to the exe; paths use forward slashes. |
| Controller not responding | Rebuild after saving `pad_map.json`; window must have focus for logging and input. |
| Buttons greyed out | Wait for the current long operation (lift/build) to finish, or cancel it. |

When reporting a problem, include:

- Game name and region
- Whether the ELF was decrypted by you
- Full log from the Log tab / named run log
- Exact steps that led to the failure

Professional GitHub issue templates are available in the repository for this purpose.

---

## 9. Command-line (advanced)

```text
ps3_cli <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>
```

Performs project creation, lifting, and host build without the GUI. Paths must point at valid runtime sources and compiler tools.

---

## 10. Safety and good practice

- Do not upload or share game dumps, encrypted or decrypted executables, firmware, keys, or extracted assets.
- Confirm that your intended processing is permitted; ownership or a legal acknowledgment alone does not guarantee that every operation is lawful.
- Back up your project folders before major experiments.
- Do not run untrusted binaries obtained from unofficial sources.
- Report security issues privately according to the project’s SECURITY policy.

---

## 11. Further reading

- [README.md](README.md) — project overview and status table  
- [ROADMAP.md](ROADMAP.md) — detailed phase status (I–XIX, XX-A…O, future work)  
- [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md) — architecture for contributors  
- Repository Support / Contributing / Code of Conduct documents  

---

## 12. Summary checklist

- [ ] I own the game and use only a legal decrypted ELF  
- [ ] I accepted the in-app TERMS  
- [ ] Loaded ELF → Decompiled → Mapped controls → Built → Copied to USRDIR → Ran  
- [ ] `zlib1.dll` is present if the game uses PSARC  
- [ ] I understand that solid blue / incomplete SPU / missing draws are still common  
- [ ] I will attach logs when asking for help  

**PPSX33 is a research toolkit — not a retail port.**  
Thank you for using it responsibly.
