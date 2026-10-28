# PPSX33 Roadmap

Evidence-based status. External code audits are leads until proven by tests or title logs.

Legend: **Done** · **Partial** · **In progress** · **Blocked** · **Planned**

---

## Current critical path

| Priority | Work | Status |
| --- | --- | --- |
| 1 | PPU past sticky / import islands; recovery respects region bans | Partial |
| 2 | Null-loop / soft_breaks escape without inventing success | Partial |
| 3 | FIFO GET/PUT truth; GET only after accept | Partial |
| 4 | Guest PUT advances without boot assist | Blocked on HLE/boot |
| 5 | Title-authored draws / shaders | Blocked on (4) |

**Evidence note:** Assist cmdstreams can yield non-zero `methods` and guest-counted draws. That validates the host RSX path only.

---

## Phase 0 — Trustworthy baseline

| Item | Status |
| --- | --- |
| Reproducible host build (ps3core / ps3rt / GUI) | Partial |
| Preserve lift_report / analysis_report per project | Done |
| `PS3RT_STRICT_FAULT` for correctness runs | Done (env) |
| Soft recovery clearly logged as non-authoritative | Partial |
| CI: build + smoke tests | Partial |
| Decrypt / external emulator launch removed from GUI | Done |

## Phase 1 — PPU translation trust

| Item | Status |
| --- | --- |
| Large instruction emission surface in `ppu_lifter.cpp` | Partial |
| Split decoder / semantics / codegen | Planned |
| No silent no-op for real ops | In progress |
| Branches, CR, load/store, XER/LR/CTR verified by tests | Planned |
| VMX correctness before host SIMD speedups | Partial |
| Differential PPU harness | Planned |

## Phase 2 — Guest runtime

| Item | Status |
| --- | --- |
| Guest memory + image load | Partial |
| Import / NID dispatch | Partial |
| Threads / sync primitives | Partial |
| Filesystem / save / audio / pad | Partial |
| Module-level HLE tests | Planned |

## Phase 3 — SPU pipeline

| Item | Status |
| --- | --- |
| Runtime interpreter + MFC tags/atomics | Partial |
| Dedicated SPU decoder + static lifter | Planned |
| DMA completion ordering tests | Planned |
| SPURS job lifecycle | Partial |

## Phase 4 — RSX

| Item | Status |
| --- | --- |
| FIFO parse + D3D11 present | Partial |
| Assist/boot cmdstream draws | Partial (probe only) |
| Title GCM stream | Blocked |
| Shader translation (VP/FP → HLSL) | Partial |
| Textures, depth, blend, RTs | Partial |
| Split cmd processor / state / backend files | Planned |
| Second backend (Vulkan/etc.) | Planned after common pipeline |

## Phase 5 — HLE coverage (measured)

Expand from title logs only. Track Complete / Partial / Stub per module (see README module table).

## Phase 6 — Optimization

Only after conformance gates: IR opts, shader cache, incremental lift, SPU native jobs.

---

## Packaging / UI

| Item | Status |
| --- | --- |
| WinForms GUI workflow | Partial |
| Prebuilt `ps3rt.dll` + game links import lib | Partial |
| Load decrypted ELF only | Done |

## Env flags

| Variable | Meaning |
| --- | --- |
| `PS3RT_STRICT_FAULT=1` | Halt on sticky/unknown PC; no invented targets |
| `PS3RT_INJECT_SEED=0` | Disable boot FIFO assist |
| (default assist) | Probe cmdstream while PUT idle — not title rendering |

See also [ENGINEERING.md](ENGINEERING.md).
