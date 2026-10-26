# PPSX33 Roadmap

**Last updated:** 2026-10-26  
**Scope:** Experimental PlayStation 3 static recompiler — research toolkit, not a retail emulator.

Phases are consolidated to **I – XV** only. Older XX+ labels are archived conceptually into these bands.

---

## Phase table

| Phase | Name | Status | Summary |
|------:|------|--------|---------|
| **I** | Project / ELF ingest | DONE | Load BE ELF64, segments, TOC |
| **II** | PPU lift surface | DONE | Chunked translation to host C++ |
| **III** | Host project build | DONE | MSVC via system Visual Studio |
| **IV** | Runtime image | DONE | Guest memory, entry, CRT glue |
| **V** | LV2 / imports HLE | IN TREE | Syscalls + PRX stubs (expand by title) |
| **VI** | SPU / MFC | IN TREE | Local store, DMA, channels |
| **VII** | Filesystem / assets | IN TREE | cellFs, PSARC, sequential load |
| **VIII** | Audio / pad / save | IN TREE | waveOut, XInput map, save basics |
| **IX** | GCM / FIFO bind | IN TREE | FIFO put/get, method dispatch |
| **X** | Host D3D11 present | DONE | Swap chain, window, flip |
| **XI** | PPU accuracy / faults | IN TREE | Strict/recovery, wild-pc controls |
| **XII** | VMX / FP honesty | IN TREE | No false “implemented” soft copies |
| **XIII** | Host UX | DONE | Native WinForms UI (no WebView2) |
| **XIV** | Guest surfaces / textures | IN TREE | Color buffer upload, textured blit |
| **XV** | **Real rendering** | **ACTIVE** | Guest geometry, shaders, no diagnostic-only frames |

Detail files: [roadmap/I.md](roadmap/I.md) … [roadmap/XV.md](roadmap/XV.md).

---

## Phase XV — Real rendering (ACTIVE)

Goal: frames produced by **guest** command streams, vertex data, and (progressively) shaders — not host diagnostic triangles counted as success.

| Sub | Status | Work |
|-----|--------|------|
| **XV-A** | DONE | Guest vs diagnostic draw counters |
| **XV-B** | IN TREE | FIFO multi-arg packets / JUMP (prior pack) |
| **XV-C** | **THIS** | Present path: guest VA first; diagnostic only if no guest draw |
| **XV-D** | NEXT | Indexed draws + multi-attribute fetch |
| **XV-E** | NEXT | SCE VP/FP decode → HLSL IR (no silent fallback success) |
| **XV-F** | NEXT | Texture formats / tilings used by target title |
| **XV-G** | NEXT | Depth/blend/viewport parity for one boot scene |

### Success criteria (XV)

1. `draws_guest_frame > 0` on a real title path without forced inject  
2. Window shows non-diagnostic content for that path  
3. Logs report `guest=` separate from `diag=`  
4. Shader binds > 0 only when guest programs were translated  

---

## Requirements

See [README.md](README.md) and [redist/README.txt](redist/README.txt).  
Toolchain: **system Visual Studio** only (`redist/` = installer + guide).

## Future

OpenGL 3.3 / 2.1 fallback when D3D11 is unavailable (post-XV).

---

*No proprietary keys or copyrighted game assets are included.*
