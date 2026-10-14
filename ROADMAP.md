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

## Remaining to implement
| ID | Item |
|----|------|
| **XX-H** | Break null-loop / soft GCM — guest past ~`0x33723c` into real cellFs + draws |
| XX-I | Full `cellGcmSys` HLE (not null import stubs) |
| XX-J | Asset streaming: PSARC zlib blocks end-to-end |
| XX-K | SPU ISA interpreter / JIT (today: mailbox completion stub only) |
| XX-L | MFC / DMA transfers + tags |
| XX-M | SPURS-like SPU scheduling / concurrency |
| XX-N | Audio path + title / UI / text on screen |
| XX-O | ICON0.PNG as window + taskbar icon when run from USRDIR |

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
