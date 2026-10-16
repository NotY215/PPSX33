# Phase XXV — Truthful execution & first real pixels (ACTIVE)

Informed by external audit (GPT) cross-checked against the tree and GOW logs.

## What the audit got right

| Claim | Verdict |
|-------|--------|
| Silent `return true` in `ppu_lifter.cpp` for empty cases | **Correct** — primary opc 0/1/2/3/5/6/9 paths emitted nothing |
| Wild-PC recovery can mask real faults | **Correct** — useful experimentally; unsafe as default compatibility |
| RSX is not a faithful renderer | **Correct** — HLSL blit/diagnostic ≠ game geometry |
| LV2/HLE success stubs risk invalid state | **Correct** — many paths return 0 |
| SPU/MFC/SPURS incomplete semantics | **Mostly correct** — many opcodes exist; DMA/atomics/SPURS approximate |

## What to treat carefully

| Claim | Note |
|-------|------|
| Full `LiftStatus` redesign | Good long-term; adapt carefully to current `lift_one` bool API |
| Stop all recovery immediately | Keep optional recovery mode; strict mode should fault with capture |
| Full RSX shader ISA in one step | Start with FIFO + surfaces + one synthetic triangle, then game streams |

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXV-A** | DONE (untested) | Fix `audio_queue_s16` C3861 (forward declare) | `audio_save.cpp` |
| **XXV-B** | DONE (untested) | Visible host frame every present (bright clear + log) | `rsx_stub.cpp` |
| **XXV-C** | DONE (untested) | Lifter: empty primary cases return **false** + marker | `ppu_lifter.cpp` |
| **XXV-D** | PLANNED | RSX: FIFO decode bounds, COLOR surface present, first guest draw path | `rsx_stub.cpp` |
| **XXV-E** | PLANNED | PPU: LiftStatus / unsupported report; branch/CR audit | `ppu_lifter.cpp` |
| **XXV-F** | PLANNED | Runtime strict fault: validate_guest_pc, capture_guest_fault | `ppu_runtime_run.h` |
| **XXV-G** | PLANNED | PRX/LV2 contracts (no silent success for unknown NID) | `ps3rt_b.inl`, `lv2_syscalls.inl` |
| **XXV-H** | PLANNED | SPU/MFC tag completion ordering | `spu_stub.cpp` |

## Recommended order (aligned with audit)

1. **XXV-A…C** — build green + visible frame + honest lift
2. **XXV-D** — real surface present / FIFO
3. **XXV-E/F** — truthful PPU + deterministic faults
4. **XXV-G/H** — HLE + SPU/DMA

FPS is not a goal until frames show correct guest content.
