# PPSX33 Roadmap — God of War 3 (GOW_3)

**Project:** PS3 ELF recompiler + HLE runtime (PPSX33)  
**Target:** Boot GOW_3 past early PRX/GCM init into real RSX present  
**Last updated:** 2026-10-12

---

## Status legend

| Tag | Meaning |
|-----|---------|
| **Done** | Landed on `main`, verified in logs or by design |
| **Partial** | Landed but still incomplete / regresses under some paths |
| **Next** | Immediate implementation target |
| **Planned** | Designed, not started |

---

## Master phase table

| Phase | Status | Focus | Key files | Success signal |
|-------|--------|-------|-----------|----------------|
| **A** | Done | Heartbeat / stall watchdog so UI does not freeze silently | `ppu_runtime_run.h` | `[ps3] heartbeat steps=...` |
| **B** | Done | Soft null-loop skip (+0x40 stride) | `ppu_runtime_run.h` | `null-loop skip: ... +0x40` |
| **C** | Done | Growing stride (+0x100 / +0x400) on sticky null LRs | `ppu_runtime_run.h` | soft_breaks climb with larger strides |
| **D** | Done | Ban sticky LRs so the same dead return is not reused | `ppu_runtime_run.h` | `ban LR=0x...` |
| **E** | Done | Stack recover for alternate return PCs | `ppu_runtime_run.h` | `stack-escape` / `deep-recover` |
| **F** | Done | Soft-break null regions; TOC restore on r2=0 | `ppu_runtime_run.h` | soft_breaks + TOC stable |
| **G** | Done | Real GCM state: control @ 0x8000000, labels, FIFO, D3D11 present | `ps3rt.cpp`, `rsx_stub.cpp` | `[gcm] Phase G state ready` + D3D11 ready |
| **H** | Done | Break `pc==lr` spin waits (0x32eb24-class) | `ppu_runtime_run.h` | spin break / force +4 |
| **I** | Done | Build fixes: SPUContext redef, CR helpers (`set_cr_*`, `bc_taken`) | `ppu_runtime_core_*.h` | MSVC build succeeds |
| **J** | Done | Stronger pc==lr wait + heartbeat progress_hw | `ppu_runtime_run.h`, `ps3rt.cpp` | progress_hw during waits |
| **K** | Done | `bc_taken` consecutive-taken force-fallthrough | `ppu_runtime_core_b.h` | `force-fallthrough bc` |
| **L** | Done | Combined internal-spin + mem-touch progress | `ps3rt.cpp` (`ps3rt_touch`) | `touch-progress n=` |
| **M** | Done | Early ban + escape to `last_good` instead of endless stride | `ppu_runtime_run.h` | `Phase M escape null-region` |
| **N** | Done | Break `last_good` ping-pong; `failed_escape` tracking | `ppu_runtime_run.h` | no repeating escape to same PC |
| **O** | Done | Break import stub ping-pong (`0x39800000` ↔ sticky lr) | `ppu_runtime_run.h` | `Phase O break stub-loop` |
| **P** | Partial | RSX flush-before-sync; LV2 usleep/time/mutex; GCM PUT bump | `ps3rt.cpp`, `lv2_syscalls.inl` | methods path fixed; **regressed early ban** |
| **Q** | Done | Fix early-boot ban: allow `lr=0x103ac` (only ban `<0x10000` or `==0x10000`) | `ppu_runtime_run.h` | first stub returns `#1 -> 0x103ac` |
| **R** | Done | Import last-resort; never ban early LRs in `0x10004..0x1FFFF` | `ppu_runtime_run.h` | no immediate `escapes=0` halt |
| **S** | Done | Emit real-ish GCM method packets (CLEAR + surface) into FIFO | `ps3rt.cpp` | `methods=` rises above 0 |
| **T** | Next | Full NID recovery from TOC/OPD/import tables | `nid_table.h`, `ps3rt.cpp` | named `[prx] cellGcmSys::...` hits |
| **U** | Planned | Decode real guest FIFO (not only synthetic NOPs) | `rsx_stub.cpp` | `draws>0` from game commands |
| **V** | Planned | Texture / surface upload path (guest VRAM → D3D) | `rsx_stub.cpp` | non-clear framebuffer content |
| **W** | Planned | Vertex / index pull + simple draw | `rsx_stub.cpp` | geometry on screen |
| **X** | Planned | Shader HLE / stub programs | new `rsx_shaders.*` | lit surfaces |
| **Y** | Planned | SPU thread schedule + MFC DMA fidelity | `ps3rt.cpp`, SPU stubs | SPU jobs complete |
| **Z** | Planned | Audio (cellAudio / libmixer HLE) + pad full path | new modules | sound + input in-game |

---

## Milestone groups

### 1. Survive boot (A–F) — **Done**
Keep the PPU from wedging on null import returns and dead LRs long enough to reach library init.

### 2. Graphics skeleton (G–J) — **Done**
Allocate GCM control/labels/FIFO, open a D3D11 window, flip/clear so the host presents *something*.

### 3. Escape correctness (K–R) — **Done / hardening**
Stop infinite soft-skips, stub loops, and bad `last_good` targets without breaking early entry (`0x10230` / `0x103ac`).

### 4. Real RSX traffic (S–W) — **In progress (S done)**
Guest (or HLE) must produce decodeable FIFO methods; host must turn draws into pixels.

### 5. Full system (X–Z) — **Planned**
Shaders, SPU, audio, complete pad — toward playable GOW_3.

---

## Current blockers (priority order)

1. **NID mostly `0x00000000`** — import stubs cannot specialize GCM calls (Phase **T**).
2. **FIFO mostly synthetic** — game may not write real put/get yet; HLE must either recover NIDs or inject plausible command streams (Phase **S/U**).
3. **`draws=0`** until DRAW_ARRAYS / INDEX methods appear (Phase **U/W**).
4. **GUI null-ref** (`Object reference not set`) is a C# host issue, separate from PPU runtime.

---

## Runtime architecture (where work lives)

```
runtime/
  ppu_runtime.h          # umbrella include
  ppu_runtime_core_a.h   # PPUContext, memory helpers
  ppu_runtime_core_b.h   # CR / bc_taken / xer helpers
  ppu_runtime_run.h      # ppu_run loop, escapes, bans, heartbeats
  ps3rt.cpp              # LV2 syscalls, PRX/GCM HLE, progress_hw
  rsx_stub.cpp           # FIFO decode, D3D10/11 present
  nid_table.h            # NID → name/kind table
  lv2_syscalls.inl       # documented extra syscall cases
  spu_stub.cpp           # SPU context stubs
```

Generated game project (per ELF):
```
build/dist/projects/GOW_3/output/
  guest_image.bin
  src/ppu_chunk_*.cpp
  src/ps3rt.cpp (copied from runtime)
```

---

## How to apply updates

```bat
cd /d E:\PPSX33
git pull origin main
```

Then in PPSX33 GUI: **Rebuild All** → **Build** → **Run**.

Do **not** manually copy runtime files; the project build copies `runtime/*` into the game output.

---

## Phase detail notes

### A–F (boot survival)
Null import PC `0x0` / `0x39800000` with sticky LRs (`0x199c78`, `0x336660`) caused soft loops. Soft strides, bans, and stack recover unblocked progress.

### G (GCM skeleton)
```
control  @ ~0x8000000
labels   @ ~0x8001000
fifo     @ ~0x8020000
```
`progress_hw` clears labels, syncs get/put, auto-flips for host present.

### N–R (escape correctness)
- **N:** `failed_escape` stops last_good ping-pong.
- **O:** same `(stub,lr)` ×4 → force `lr+4`.
- **P:** flush **before** get=put (methods were always 0).
- **P regression:** banned all PC `< 0x11000` → fixed in **Q/R**.

### S (this commit)
On each `cellGcmSys` HLE entry, write a small method sequence into the FIFO:
- `NV4097_CLEAR_SURFACE`
- surface clip / viewport style packets  
so `rsx_process_fifo` increments `methods_seen`.

### T–Z (forward)
Real NIDs unlock real GCM function behavior; real FIFO unlocks draws; draws + textures unlock visible gameplay; SPU/audio complete the system.

---

## Definition of done (GOW_3)

| Level | Criteria |
|-------|----------|
| Boot | Past first 50 import stubs without halt; heartbeats on real code PCs |
| Present | Continuous D3D flips; TOC stable (`r2=0x52d6c8`) |
| Methods | `methods_seen > 0` sustained |
| Draws | `draws > 0` from decoded FIFO |
| Image | Non-clear color buffer content |
| Playable | Pad + audio + no hard spin for minutes |

---

## Changelog (recent)

| Date | Phase | Note |
|------|-------|------|
| 2026-10-11 | G–P | GCM, spins, flush order, LV2 waits |
| 2026-10-11 | N–O | last_good + stub ping-pong |
| 2026-10-12 | Q–R | Early-boot ban fix |
| 2026-10-12 | S | Synthetic GCM method packets + this roadmap |
