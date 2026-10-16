# Phase XXIII — Execution reality (ACTIVE)

Log reference: `docs/games/GOW III/ppsx33_log_in_game_folder_2026-10-16_14-13-47.txt`

Surfaces **A–Z** are in the tree. Runtime evidence from that log is mixed: GCM/FIFO and present loop run; **guest draw content and HLSL path do not yet produce a game frame**. Audio path is spamming failures.

---

## Subphases (detailed)

| ID | Status | Focus | Primary files | Log evidence (2026-10-16) |
|----|--------|-------|---------------|--------------------------|
| **A** | DONE | Null-PC recovery to last-good / chunk | `ppu_runtime_run.h` | wild-pc recoveries present |
| **B** | DONE | Present every flip | `rsx_stub.cpp` | flips #1…1281+ |
| **C** | DONE | Audio host keep-alive | `audio_save.cpp` | **broken in practice** — see below |
| **D** | DONE | PPU opcode markers | lifter | not isolated in this log |
| **E** | DONE | RSX present + FIFO methods | `rsx_stub.cpp` | methods counter moves |
| **F** | DONE | SPU/MFC tag/stall | `spu_stub.cpp` | little SPU noise in this log |
| **G** | DONE | LV2 wait/signal contracts | `lv2_syscalls.inl` | few `[lv2]` lines |
| **H** | DONE | PRX NID recovery | `ps3rt_b.inl` | many `XXIII-H prx_import` **mod=?** |
| **I** | DONE | cellFs / PSARC miss logging | `assets.cpp` | PSARC scan OK |
| **J** | DONE | SPURS job slice | `spu_stub.cpp` | not dominant in log |
| **K** | DONE | Timing / GET=PUT assist | `ps3rt_b.inl` | `[time]` lines present |
| **L** | DONE | Format identity + open coverage | `assets.cpp` | few `[fs]` |
| **M** | DONE | FIFO drain / GET commit (deep) | `rsx_stub.cpp`, `ps3rt_b.inl` | `guest put=0x20 get=0x20` |
| **N** | DONE | Vertex attr tracking | `rsx_stub.cpp` | shaders/tex still 0 |
| **O** | DONE | VP/FP bind surface | `rsx_stub.cpp` | shaders=0 on flips |
| **P** | DONE | Texture pitch / expand upload | `rsx_stub.cpp` | tex=0 |
| **Q** | DONE | Extra GCM/sysutil NIDs | `nid_table.h`, `ps3rt_b.inl` | imports still kind=0 |
| **R** | DONE | TOC/OPD / wild-pc | `ppu_runtime_run.h` | `XXIII-R wild-pc 0x0 -> 0x199c78` |
| **S** | DONE | Thread create HLE | `lv2_syscalls.inl` | not isolated |
| **T** | DONE | HLSL VS/PS blit (D3DCompile) | `rsx_stub.cpp` | **no** `HLSL` log lines in this run |
| **U** | DONE | DXT/BC detect + fallback | `rsx_stub.cpp` | no DXT lines |
| **V** | DONE | Host textured quad on draw | `rsx_stub.cpp` | draws≈1–2 from inject |
| **W** | DONE | Save/sysutil tags | `audio_save.cpp` / HLE | `[save]` once |
| **X** | DONE | Diagnostic / force frame path | `rsx_stub.cpp` | tag on flips; **no** `diagnostic frame` line |
| **Y** | DONE | HLSL sample path (not full RSX ISA) | `rsx_stub.cpp` | no compile/draw HLSL logs |
| **Z** | DONE | Pitch align; color/tex present | `rsx_stub.cpp` | present loop alive |

---

## Log analysis (2026-10-16_14-13-47)

### What works
- PSARC inventory of USRDIR archives (zlib path loaded).
- GCM state: control `0x8000000`, fifo `0x8020000`, labels `0x8001000`.
- Window + flip loop (`XXIII-X/Y/Z flip #…`).
- XXIII-R wild-PC recovery fires.
- XXIII-H import stub path fires (often).

### Critical blockers
1. **Audio storm** — ~20k lines of `[audio] G/K waveOutOpen failed — silent HLE`. Open is retried every tick after failure. Must fail-once and stop logging/retrying.
2. **Guest FIFO stuck at put=get=0x20** — not a full command stream. Later flips show `methods=131076` with `shaders=0 tex=0` → drain/count inflation without real graphics state.
3. **No HLSL / diagnostic evidence** — expected `[rsx] XXIII-T…` / `XXIII-X diagnostic frame` absent → present path may clear without upload, or D3DCompile/device path not taken.
4. **Import NID quality** — NIDs like `0x4808F661` with `mod=?` look like addresses/TOC noise, not resolved library exports. Boot never reaches code that fills the FIFO with real draws.
5. **Overlay stats** — `draws=1..2`, `methods=4` early then huge method counts; still **no textures/shaders**.

### Tag volume (approx)
| Tag | Count | Note |
|-----|------:|------|
| `[audio]` | ~20608 | failure spam |
| `[rsx]` | ~77 | flips |
| `[ps3]` | ~76 | imports / wild-pc |
| `[psarc]` | ~31 | archive scan |
| `[gcm]` | ~14 | control + put/get |

---

## Next patches (priority) — proposed **Phase XXIV**

Do **not** add more lettered XXIII surface until these are fixed against a new log:

| Priority | Patch | Why |
|---------|--------|-----|
| **P0** | **Audio open once** — on `waveOutOpen` failure set permanent silent flag; no per-tick reopen/log | Log is unusable; CPU waste |
| **P0** | **FIFO truth** — treat put/get `0x20` as empty/minimal; do not spin-drain to 131k “methods”; only decode when put advances past real command words | Fake method counts hide progress |
| **P0** | **HLSL/diagnostic prove-out** — log D3DCompile success/fail once; force one gradient upload if `tex=0` after first flips | Confirm present pipeline |
| **P1** | **PRX NID decode** — stop accepting TOC-range values as NID; bind `cellGcm*` / sysmodule exports used at boot | Unblocks guest putting real commands |
| **P1** | **Import stub return** — return to LR/OPD correctly so boot continues past stub PC `0x39800000` | Control flow after HLE |
| **P2** | **Color buffer present** — if `COLOR_OFFSET` set, blit that region every flip even when shaders=0 | First non-solid *game* pixels |
| **P2** | **Reduce inject dependency** — prefer guest fifo when put grows; keep inject only when put stays flat | |

Suggested phase label: **XXIV — Boot-to-first-pixels** (A audio, B FIFO, C HLSL prove, D PRX, E color blit).

---

## When a real game frame is plausible

| Checkpoint | Signal in log |
|------------|----------------|
| Host path alive | `XXIII-T HLSL … ready` or `diagnostic frame` without audio spam |
| Guest commands | `guest put` grows well beyond `0x20`; methods track *new* commands |
| Content | `tex>0` or COLOR_OFFSET blit; shaders may still be 0 |
| Boot progress | fewer `mod=?` imports; fewer wild-pc to same address |

Full scenes still need deeper shader ISA (beyond blit) and title-specific HLE — after XXIV P0/P1.

---

## GUI note

Build-complete MessageBox must stay as concatenated string literals (no raw newlines / en-dash inside quotes).
