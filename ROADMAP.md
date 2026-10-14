# PPSX33 Roadmap

## Earlier phases (I–XIX) — testing
Core recompiler, HLE stubs, RSX host present, audio/pad hooks, build pipeline.
These are **testing** quality — not final. Regressions can still appear.

## Package phase (XX-A … XX-G+) — done
| ID | Item |
|----|------|
| XX-A | Game-named EXE + `ps3rt.dll` + `guest_image.bin` + `pad_map.json` |
| XX-B | cellFs + PSARC TOC / zlib blocks (partial) |
| XX-C | Copy into `PS3_GAME/USRDIR` (auto-find EBOOT.BIN) |
| XX-D | Controller map → `pad_map.json` |
| XX-E | Legal / TERMS first-run (links open in system browser) |
| XX-F | Post-build layout (reports at project root; runtime in `output/`) |
| XX-G | Named run logs; pad log only when game window focused |

## Phase XX-H … XX-O — implemented (apply + test)
| ID | Item |
|----|------|
| **XX-H** | **DONE** — null-loop skip, GCM flip/wait HLE |
| **XX-I** | **DONE** — fuller cellGcmSys HLE (display buffers, map, tiles) |
| **XX-J** | **DONE** — PSARC TOC + zlib block streaming |
| **XX-K** | **DONE** — SPU ISA interpreter (LS/arith/branch/channel) |
| **XX-L** | **DONE** — MFC list DMA, barriers, atomics, tags |
| **XX-M** | **DONE** — SPURS job queue + prio schedule + cellSpurs HLE |
| **XX-N** | **DONE** — cellAudio AddData→waveOut, GDI status overlay, title |
| **XX-O** | **DONE** — ICON0.PNG window/taskbar icon from PS3_GAME |

## Solid blue screen (current symptom)
Log shows RSX flips / clears while PPU hits null-loop soft_breaks at ~`0x33723c`, then halt `pc=0x0`.
Typical causes in this codebase:
1. Incomplete GCM / import stubs → guest waits forever  
2. Thread / flip timing vs logic  
3. Assets not reached (`[fs]` / `[psarc]` absent until cellFs is really called)

**Full SPU ISA + MFC + SPURS is multi-week work**, not a single patch. Track stalls via GitHub issue templates + attach the latest run log.

## Workflow
1. Load decrypted ELF (or Decrypt EBOOT via your RPCS3)  
2. Decompile (lift chunks)  
3. Set controller keys (before Build)  
4. Build → `<GameName>.exe` + `ps3rt.dll` in `output/`  
5. Copy to game folder (into USRDIR next to EBOOT.BIN)  
6. Run the game-named EXE from USRDIR  

PPSX33 is a research toolkit — not a retail port.
