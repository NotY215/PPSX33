# PPSX33 Roadmap

Evidence-based status. External AI notes are leads only until a log or test proves them.

Legend: **Done** · **Partial** · **In progress** · **Blocked** · **Planned**

---

## Current critical path (do first)

Ordered by what actually blocks title rendering today.

| Priority | Work | Status |
| --- | --- | --- |
| 1 | PPU past sticky / import islands (first-fault, no invented PC in strict mode) | Partial |
| 2 | FIFO GET/PUT truth (no false resync; GET only after accept) | In progress |
| 3 | Guest PUT advances without boot assist | Blocked on boot/HLE |
| 4 | Title-authored draws / shaders / textures from real GCM stream | Blocked on (3) |
| 5 | Broader LV2 / PRX / NID on the fault path only | Partial |

**Recent evidence:** boot FIFO assist can produce non-zero `methods` and guest-counted draws through the host RSX path. That proves the **decoder → present** chain. It does **not** prove title meshes. Assist is a probe; title PUT is the goal.

---

## Packaging / UI

| Item | Status |
| --- | --- |
| Simple C# controller mapping (no WebView, no native DLL, no animations) | Done |
| Native controller DLL removed from CMake | Done |
| Decrypt / external emulator launch removed from GUI | Done |
| Load decrypted ELF only | Done |
| `dist/runtime` packaging / prebuilt `ps3rt` | Partial |

## P0 — PPU first-fault sticky BREAK

| Item | Status |
| --- | --- |
| Capture fault snapshot on sticky BREAK | Done |
| Soft escalate / skip-banned / dead-island handling | Partial |
| `PS3RT_STRICT_FAULT=1` halts instead of inventing PC | Done |
| Originating branch/import fixed from sticky log | In progress |

## P0 — FIFO GET after accept

| Item | Status |
| --- | --- |
| Do not force GET=PUT on large gap | Done |
| PUT-restart when GET is stale-ahead of low PUT | In progress |
| Progress path syncs GET from `ps3rt_rsx_get_get()` after flush | Done |
| BE-preferring FIFO header parse | In progress |

## P1 — Guest draw without assist seed

| Item | Status |
| --- | --- |
| Boot FIFO assist (probe) when PUT idle | Partial (default on; `PS3RT_INJECT_SEED=0` disables) |
| Title-authored GCM for real meshes | Blocked on boot/HLE |
| When guest PUT advances, verify non-assist DRAW in log | Planned |

---

## Longer-term technical plan

How PPSX33 intends to deepen the stack **after** the critical path above is honest. These are engineering directions, not claims of current completion.

### 1. PPU and code reconstruction

| Direction | Intent | Status |
| --- | --- | --- |
| Stronger CFG / OPD / prologue boundary recovery | Better static function and call-graph recovery | Planned |
| Indirect branch / jump-table resolution | Reduce wild-PC and missed call targets | Planned |
| Optional richer IR (e.g. LLVM) for host optimization | Later performance path; not a substitute for correct runtime | Planned |
| VMX/AltiVec correctness first, then host SIMD mapping | Correctness before AVX/NEON speedups | Partial |

### 2. SPU, MFC, and SPURS

| Direction | Intent | Status |
| --- | --- | --- |
| Deeper MFC (list DMA, tags, barriers, atomics) | Match Cell transfer semantics titles rely on | Partial |
| Channel / mailbox fidelity | SPU↔PPU signalling | Partial |
| SPURS job assist → denser scheduling when titles need it | Avoid overbuilding before titles hit SPURS | Partial |
| Optional worker-pool / batched DMA host paths | Performance once behaviour is correct | Planned |

### 3. HLE framework

| Direction | Intent | Status |
| --- | --- | --- |
| Expand LV2 / library HLE only for observed call sites | Evidence-driven, not blanket stubs | In progress |
| NID/export tables maintained for modules we actually use | Traceable imports | Partial |
| Filesystem, audio, pad, save paths validated per title | Service completeness | Partial |
| Flat guest memory / lower endian friction | Host mapping quality | Partial |
| Fiber-style scheduling only if titles require it | Avoid speculative complexity | Planned |

### 4. RSX / GPU backend

| Direction | Intent | Status |
| --- | --- | --- |
| NV4097 FIFO parser + state machine | Consume real command streams | Partial |
| D3D11 present and draw path | Host output | Partial (assist draws proven) |
| Vertex fetch / indexed draws from guest buffers | Real geometry | Partial |
| Fragment/vertex program translation (HLSL path) | Title shaders | Partial |
| Textures, RTs, depth/blend | Scene fidelity | Partial |
| Optional later: Vulkan/DX12 or offline shader pipelines | Portability / stutter reduction | Planned |
| ZCULL / tile-style optimizations | Only after depth path is real | Planned |

### 5. Tooling and developer experience

| Direction | Intent | Status |
| --- | --- | --- |
| GUI project flow (load → lift → build → run) | Usable research workflow | Partial |
| Lift / analysis reports per game under `docs/games` | Evidence store | Partial |
| Richer ELF/CFG inspection tools | Debug unresolved branches | Planned |
| Native debugger attach to generated EXE | Host-side debug | Planned |

---

## Working rules

1. **Log first** — change what the latest title log proves is broken.  
2. **No fake wins** — assist/diagnostic draws are not title rendering.  
3. **Strict mode available** — `PS3RT_STRICT_FAULT=1` for deterministic faults.  
4. **Long-term plan does not reorder P0** — CFG/LLVM/Vulkan do not jump ahead of sticky + guest PUT.

## Env flags

| Variable | Meaning |
| --- | --- |
| `PS3RT_STRICT_FAULT=1` | Halt on sticky/unknown PC; no invented targets |
| `PS3RT_INJECT_SEED=0` | Disable boot FIFO assist |
| `PS3RT_INJECT_SEED=1` / default assist | Allow diagnostic/boot cmdstream while PUT idle (probe only) |
