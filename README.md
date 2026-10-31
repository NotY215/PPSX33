<!-- PPSX33 project identity -->
<p align="center">
  <img src="assets/ppsx33-logo.png" alt="PPSX33 logo" width="150">
</p>

<h1 align="center">PPSX33</h1>
<p align="center"><strong>PlayStation 3 Full Recompiler</strong></p>
<p align="center">A native Windows recompiler focused on translating PS3 game code and rebuilding the runtime needed to execute it.</p>

<p align="center">
  <img src="assets/ppsx33-banner.png" alt="PPSX33 banner" width="100%">
</p>

<p align="center">
  <a href="https://ppsx33.xo.je/">🌐 Website</a> ·
  <a href="ROADMAP.md">🧭 Roadmap</a> ·
  <a href="docs/ARCHITECTURE.md">🏗️ Architecture</a> ·
  <a href="docs/DIAGRAMS.md">📊 Diagrams</a> ·
  <a href="ENGINEERING.md">📐 Engineering</a> ·
  <a href="docs/DEVELOPER_GUIDE.md">🛠️ Developer guide</a> ·
  <a href="USER_MANUAL.md">📘 User manual</a> ·
  <a href="docs/games/README.md">🎮 Game notes</a>
</p>

<p align="center">
  <a href="https://ppsx33.xo.je/"><img alt="Website" src="https://img.shields.io/badge/website-ppsx33.xo.je-00B4D8?style=for-the-badge"></a>
  <img alt="Platform: Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-0078D4?style=for-the-badge">
  <img alt="Graphics: Vulkan 1.2" src="https://img.shields.io/badge/graphics-Vulkan%201.2-AC162C?style=for-the-badge">
  <img alt="Goal: Full Recompilation" src="https://img.shields.io/badge/goal-full%20recompilation-6A5ACD?style=for-the-badge">
</p>

> [!IMPORTANT]
> **PPSX33's primary goal is full PlayStation 3 game recompilation into native Windows builds.** This is the project's intended direction, not a claim that every PS3 title is already compatible. Full recompilation requires accurate PPU and SPU execution, system-library and kernel behaviour, DMA, and real RSX/GCM rendering. A successful lift or build alone does not prove that a game boots or is playable. Control-flow recovery and boot FIFO assist are diagnostic; use `PS3RT_STRICT_FAULT=1` when validating correctness.

> [!NOTE]
> **Input format:** PPSX33 loads **decrypted ELF** files only. It does **not** decrypt EBOOTs, bundle keys, or launch external emulators. Obtaining a decrypted ELF is the user's responsibility (for example, offline tooling some users already have — not provided by this project).

## ✨ What PPSX33 is building

PPSX33 is an open-source PlayStation 3 recompiler whose main objective is **full recompilation**: translate a game's PowerPC code and supporting execution paths into a native Windows build, with a host runtime that reproduces the PS3 system services and hardware behaviour the title depends on.

This is a whole-system engineering target, not just an ELF-to-C++ conversion. The project must progressively cover instruction semantics, control flow, kernel and library services, SPU/MFC/DMA behaviour, and RSX/GCM graphics so that generated builds can execute real games correctly. Implementation status is reported explicitly below and in the roadmap.

### 🔁 The pipeline

<p align="center">
  <strong>Decrypted ELF</strong> &nbsp; → &nbsp;
  <strong>🧭 Analyse</strong> &nbsp; → &nbsp;
  <strong>⚙️ Lift to C++</strong> &nbsp; → &nbsp;
  <strong>🔗 Link runtime</strong> &nbsp; → &nbsp;
  <strong>🧪 Validate</strong>
</p>

1. **Load** a decrypted ELF or supported game layout that you are legally entitled to use.
2. **Analyse and lift** supported PPU instruction regions into generated C++.
3. **Resolve runtime dependencies** through the current LV2, PRX and library HLE layers.
4. **Build** a native Windows executable using the installed Microsoft C++ toolchain.
5. **Measure what actually works** using traces, lift reports and per-game test notes.

The last step matters: generated code is only one milestone. Correct execution and rendered gameplay must be demonstrated separately.

## 🚦 Full-recompilation status

PPSX33 is being developed toward full recompilation. These statuses describe implemented subsystems and remaining gaps; they do not imply universal game compatibility.

| Area | Status | Current scope |
| --- | --- | --- |
| 🧰 GUI, project flow and build orchestration | 🟡 **Partial / evolving** | Windows GUI and native build pipeline; title-specific failures remain possible |
| 📦 ELF loading and PPU lifting | 🟡 **Partial** | Supported code can be lifted; translation coverage is not semantic correctness |
| 🧠 PPU runtime and control flow | 🟡 **In progress** | Guest context and execution paths exist; unsupported or inaccurate paths can break execution |
| 🪟 Vulkan 1.2 host window and presentation | 🟢 **Implemented baseline** | Async HWND pump + swapchain clear + guest RGBA upload |
| 🧱 RSX/GCM command processing | 🟡 **In progress** | FIFO decode and Vulkan present; assist draws proven; title-authored GCM and shaders still incomplete |
| 🧩 LV2, PRX and NID services | 🟡 **Partial** | Static-style stub binds (LR/PC→NID); runtime recovery fallback; coverage still incomplete |
| ⚡ SPU, MFC/DMA and SPURS | 🟡 **Partial** | Assist/runtime paths exist; full instruction and scheduling behaviour is not complete |
| 🔊 Audio, 🎮 input and 💾 save services | 🟡 **Partial** | Basic host paths exist; title-level behaviour still needs verification |
| 🎯 End-to-end game compatibility | 🔴 **Not established generally** | Static lift reports do not establish bootability or playability |

### Host graphics (current)

- **Default:** Vulkan 1.2 (dynamic `vulkan-1.dll` load; CMake copies DLL into `dist/`).
- **Secondary:** OpenGL (planned parity).
- **Removed:** Direct3D 11 backend and DX11 dependencies.
- **Present path:** Dedicated HWND pump thread so the game window stays responsive during boot HLE; PPU thread does not block on `vkQueuePresent`.

**Legend:** 🟢 implemented baseline · 🟡 partial or under active development · 🔴 not established / major work remains

## 📚 Module status (HLE / runtime)

HLE and host services written for this project. Public emulator behaviour may be used as a **reference for expected results**, not as a code source. Counts are approximate; individual titles can still fail on “Complete” rows if a code path is wrong or incomplete.

| Category | Modules (scope) | Status |
| --- | --- | --- |
| Kernel threading | PPU thread / mutex / cond / semaphore HLE | 🟡 Partial |
| Kernel events / timers | Event queues, timer syscalls | 🟡 Partial |
| Kernel memory | sys_memory-style alloc, mmapper subset | 🟡 Partial |
| Filesystem | cellFs / path mapping, real host I/O | 🟡 Partial |
| Save data | cellSaveData / PARAM.SFO paths | 🟡 Partial |
| Input | cellPad mapping (XInput/host keys) | 🟡 Partial |
| Audio | cellAudio host path (waveOut-style) | 🟡 Partial |
| Video out / GCM | cellVideoOut, cellGcmSys control/FIFO kick | 🟡 Partial |
| Graphics (RSX) | FIFO decode, Vulkan 1.2 present; assist draws proven; title stream pending | 🟡 In progress |
| SPU / MFC / SPURS | Interpreter, DMA/tags, job assist | 🟡 Partial |
| PRX / NID / LV2 | Import stubs, common syscalls | 🟡 Partial |
| Codecs / media | PNG/JPG helpers; ATRAC/VDEC deep decode | 🔨 Limited / partial |
| Network / NP | Not a product goal — stubs only if present | ⚪ Out of scope / stub |
| USB / Camera / Move | Not required for core recomp path | ⚪ Out of scope / stub |

**Legend:** 🟡 partial · 🔨 limited · ⚪ out of scope for current goals · 🟢 only when end-to-end proven on titles

Task-level tracking: [ROADMAP.md](ROADMAP.md).

For the task-level breakdown and longer-term architecture plan, see **[ROADMAP.md](ROADMAP.md)** and **[PPU coverage notes](docs/PPU_COVERAGE.md)**. Game-specific evidence and logs are collected under **[docs/games](docs/games/README.md)**.

## 🏗️ Architecture & documentation

| Document | Description |
| --- | --- |
| **[Website](https://ppsx33.xo.je/)** | Project site |
| **[Architecture](docs/ARCHITECTURE.md)** | Containers, runtime layers, PPU/RSX pipelines (Mermaid, PlantUML, D2, C4, …) |
| **[Diagrams index](docs/DIAGRAMS.md)** | Mermaid, PlantUML, Graphviz, WaveDrom, Vega-Lite, Nomnoml, Markmap, Kroki, … |
| **[Roadmap](ROADMAP.md)** | Phased gates and status |
| **[Engineering](ENGINEERING.md)** | Build and contributor notes |
| **[Game runtime requirements](requirements.md)** | Supported graphics APIs, display output and runtime hardware requirements |

High-level flow:

```mermaid
flowchart LR
  ELF[Decrypted ELF] --> Lift[PPU lift]
  Lift --> Build[MSVC game EXE]
  Build --> RT[ps3rt runtime]
  RT --> VK[Vulkan 1.2 present]
```

## 🧩 Why this is difficult

The PS3 combines multiple processors and a custom graphics system. A useful recompiler has to reproduce the observable behaviour games depend on, not just translate their main CPU instructions.

| Component | Responsibility | Compatibility work |
| --- | --- | --- |
| **PPU** | 64-bit PowerPC main processor | Instruction semantics, branches, registers, memory and calling conventions |
| **SPU** | Synergistic processors | Vector operations, local store, channels and job execution |
| **MFC / DMA** | SPU data movement | Transfer ordering, tags, stalls and completion semantics |
| **RSX / GCM** | Graphics processor and command interface | FIFO, shaders, vertex data, textures, render targets and synchronization |
| **LV2 / PRX** | Kernel and modules | Threads, memory, events, imports, exports and module lifecycle |
| **cellFs and other libraries** | Game-facing system APIs | Correct paths, data formats, errors, timing and state |

A failure that looks like a graphics bug may actually be caused by a missing import or an SPU transfer that completed too early. Diagnostics should preserve that chain of evidence.

## 🏗️ Project layout

See **[STRUCTURE.md](STRUCTURE.md)** for the full tree. Summary:

```text
PPSX33/
├── src/
│   ├── core/              # ELF loaders, VFS, memory, HLE kernel
│   ├── processor/PPU|SPU  # Lift, run headers, SPU interpreter
│   ├── RSX/               # Graphics pipeline + Vulkan 1.2 (OpenGL secondary)
│   ├── games-stuff/       # Audio + Video subsystems
│   ├── UI/                # WinForms GUI
│   └── cli/               # CLI tooling
├── assets/ docs/ redist/
├── STRUCTURE.md
├── ROADMAP.md
└── README.md
```

Runtime **sources** live under `src/` (no top-level `runtime/` tree).
Built artefacts in `build/dist/`:
- **`ps3rt.dll` / `ps3rt.lib` / `vulkan-1.dll` / `PS3Recomp.exe`** — next to each other at `dist/` root (single location)
- **`dist/runtime/`** — headers only for game link (no DLL/LIB duplicate)

Generated projects may contain a game executable, `ps3rt.dll`, guest image data, generated source files, controller mapping and build logs. Exact output depends on the project and build path.

## 🖥️ Game runtime requirements

The following requirements apply to **running a recompiled game** (the generated game executable with `ps3rt.dll`). They are separate from the Visual Studio, .NET and CMake tools used to build PPSX33 itself.

### Supported graphics APIs

| API | Version | Role |
| --- | --- | --- |
| **Vulkan** | **1.2** | Default host renderer. `vulkan-1.dll` must be beside the generated game executable. |
| **OpenGL** | **4.3** | Secondary core-profile path. Feature parity with Vulkan is in progress. |
| Direct3D 11 | — | **Not supported.** The D3D11 runtime and dependencies have been removed. |

### Minimum requirements

These are the current minimums for opening the game window and clearing/presenting a frame. They do not guarantee that a particular game will boot or render correctly.

| Component | Requirement |
| --- | --- |
| Operating system | 64-bit Windows 10 or later |
| GPU | A driver exposing **Vulkan 1.2** or **OpenGL 4.3** |
| Vulkan loader | `vulkan-1.dll` beside the generated game executable when using Vulkan |
| Display output | Fixed **720×576 (576p)** host present for the current path |
| Monitor | A physical display of **720×576** or higher is recommended |
| VRAM | Enough for a 576p swapchain and staging buffers; typically **1 GB or more usable** |
| Input | Keyboard or XInput controller, where supported by the title |
| Game data | Lawfully obtained title files that you are authorized to run |

### Recommended configuration

| Component | Recommendation |
| --- | --- |
| GPU | Discrete GPU with current vendor drivers and Vulkan 1.2 support |
| VRAM | **2 GB or more** |
| System memory | **8 GB or more** for guest and host buffers |
| Storage | SSD for game data and logs |

These are renderer and presentation requirements, not compatibility guarantees. Game behaviour still depends on the implemented PPU, SPU, RSX/GCM and system-service paths. A machine meeting the minimum requirements may open and present a window while a specific title remains unsupported or fails during execution.

### Building PPSX33

The requirements above are **not** the development-toolchain requirements for building the recompiler or its GUI. For the current build setup, install the Visual Studio C++ desktop development tools and Windows SDK, the .NET SDK required by the GUI project, and the CMake/Ninja tools required by the relevant targets. Check the [redistributable and toolchain notes](redist/README.txt) and the project build files for configuration-specific details.

See [requirements.md](requirements.md) for the maintained game-rendering requirements.

## 🚀 Build and try the tools

1. Install Visual Studio with the C++ desktop development tools and a Windows SDK.
2. Install the .NET SDK required by the GUI project.
3. Clone this repository and open a terminal at the repository root.
4. Run the repository build helper:

   ```bat
   scripts\\build_all.bat
   ```

5. Launch the generated GUI if the build completes successfully.
6. Load only a decrypted ELF or game layout you are legally entitled to use.
7. Review the lift report and build log before attempting to run the generated executable.

Build outputs and paths may vary by configuration. If a step fails, capture the complete error and toolchain version instead of assuming the project built successfully. The [developer guide](docs/DEVELOPER_GUIDE.md) and [user manual](USER_MANUAL.md) provide more detail.

## 🧪 How full recompilation is evaluated

PPSX33 tracks the full-recompilation pipeline in separate, verifiable stages:

- **Decode/lift coverage:** which instruction encodings the tool recognises or translates.
- **Semantic correctness:** whether translated instructions reproduce expected guest state.
- **Runtime service coverage:** whether required imports and system calls behave correctly.
- **Graphics correctness:** whether real game commands produce the expected image.
- **Game testing:** whether a documented path boots, accepts input and runs reliably.

A percentage in a lift report is not a compatibility rating. A title should only be described as playable when that claim is backed by reproducible end-to-end testing. See [PPU coverage](docs/PPU_COVERAGE.md) and the individual reports in [game documentation](docs/games/README.md).

## 🧭 Full-recompilation priorities

The engineering target is to close the gaps between translated code and a working native game build:

1. Expand PPU translation and validate instruction semantics, branches, register state, memory behaviour and control flow.
2. Replace incomplete runtime paths with correct LV2, PRX, NID and cell-library implementations.
3. Implement SPU execution and accurate MFC/DMA ordering, tags, stalls and synchronization.
4. Complete the RSX/GCM path, including FIFO processing, shaders, vertex attributes, textures, surfaces, depth, render targets and presentation.
5. Validate generated native builds with repeatable per-title boot, input, audio and rendering tests.
6. Add regression coverage and keep unsupported behaviour visible instead of silently treating it as successful execution.

The live task list is maintained in **[ROADMAP.md](ROADMAP.md)**. Contributions should include test evidence or a clear explanation of what remains unverified.

## 🕹️ Controller mapping

PPSX33 includes a controller mapping reference for the current input path:

The repository includes a controller-mapping reference in `assets/PPSX33 PS3 Controller Key Mapping.png`. Its asset provenance and any third-party marks/artwork should be verified before redistribution. See [KEY_MAPPING.md](docs/KEY_MAPPING.md) for the text reference.

See [KEY_MAPPING.md](docs/KEY_MAPPING.md) for the text reference. Actual game input support may depend on the title and runtime path.

## 🤝 Contributing

Useful contributions include:

- PPU instruction implementations with focused regression tests.
- Correct LV2/PRX behaviour backed by logs or API references.
- RSX command and shader work demonstrated with real test scenes.
- SPU and DMA tests that verify data and completion ordering.
- Reproducible Windows build fixes and clearer diagnostics.
- Game reports that distinguish “translated”, “boots”, “reaches menu” and “playable”.

Please keep changes scoped, explain assumptions, and do not add copyrighted game binaries, firmware or extracted game assets. Read [CONTRIBUTING.md](CONTRIBUTING.md) before opening a change.

## 🔗 Project references

- 🌐 [Website](https://ppsx33.xo.je/)
- 🧭 [Roadmap](ROADMAP.md)
- 🏗️ [Architecture](docs/ARCHITECTURE.md)
- 📊 [Diagrams](docs/DIAGRAMS.md)
- 🛠️ [Developer guide](docs/DEVELOPER_GUIDE.md)
- 📘 [User manual](USER_MANUAL.md)
- 🧪 [PPU coverage report](docs/PPU_COVERAGE.md)
- 🎮 [Game notes and test logs](docs/games/README.md)
- ⌨️ [Controller key mapping](docs/KEY_MAPPING.md)
- 🖥️ [Game runtime requirements](requirements.md)
- 🧰 [Toolchain setup](redist/README.txt)
- 🛡️ [Security policy](SECURITY.md)
- 📜 [Legal notes](LEGAL.md)
- 🧾 [Third-party notices](THIRD_PARTY.md)

## ⚖️ Legal and preservation

PPSX33 does not bundle commercial game files, firmware, or proprietary keys. Its optional RPCS3 integration can invoke an external EBOOT decryption workflow; this does not establish permission to process a file or guarantee legality. Users must review applicable law and third-party terms. Asset provenance and license compatibility must be checked separately. See [LEGAL.md](LEGAL.md), [TERMS.md](TERMS.md) and [NOTICE](NOTICE).

## 📄 License

See [LICENSE](LICENSE) for the project's license. Third-party components, headers and SDKs remain subject to their own licenses.
