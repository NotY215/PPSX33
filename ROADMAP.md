# PPSX33 Roadmap

Evidence-based status. AI audits are leads only.

Legend: **Done** · **Partial** · **In progress** · **Blocked**

## Packaging / UI

| Item | Status |
| --- | --- |
| Simple C# controller mapping (no WebView, no native DLL, no animations) | Done |
| Native `PPSX33ControllerNative` removed from CMake | Done |
| `dist/runtime` headers/inl only (no full source tree copy) | Done |
| Prebuilt `ps3rt` when built | Partial (game links prebuilt path) |

## P0 — PPU first-fault sticky BREAK

| Item | Status |
| --- | --- |
| Capture fault snapshot on sticky BREAK | Done |
| No region-ban / stride / chunk roulette on sticky path | Done |
| `PS3RT_STRICT_FAULT=1` halts instead of inventing PC | Done |
| Soft mode: LR-only continuation or halt | Done |

## P0 — FIFO GET after accept

| Item | Status |
| --- | --- |
| Do not force GET=PUT on large gap | Done |
| Inject path does not force GET=PUT; uses RSX consumer GET | Done |
| Progress path syncs GET from `ps3rt_rsx_get_get()` after flush | Done |

## P1 — Guest draw without inject seed

| Item | Status |
| --- | --- |
| Inject seed **off** by default | Done |
| Enable diagnostic seed with `PS3RT_INJECT_SEED=1` | Done |
| Title-authored GCM still required for real meshes | Blocked on boot/HLE |

## Next

1. Use first-fault sticky log to fix originating branch/import.
2. When guest PUT advances, verify non-inject DRAW in log.
3. Expand NID/HLE only for call sites on the fault path.

## Env flags

| Variable | Meaning |
| --- | --- |
| `PS3RT_STRICT_FAULT=1` | Halt on sticky/unknown PC; no invented targets |
| `PS3RT_INJECT_SEED=1` | Allow diagnostic clear+triangle inject |
