# PPSX33 Engineering Direction

Source-focused plan. External audits are leads only until logs or tests prove a claim.

## What “done” means

A feature is **done** only when:

1. Behavior matches PS3-visible semantics for the cases we claim, and  
2. A test or title log can fail if that behavior regresses.

Lift counts, successful compiles, and diagnostic draws are **not** completion.

## Critical path (current evidence)

From GOW III runtime logs (Oct 2026):

| Priority | Work | Status |
| --- | --- | --- |
| P0 | PPU control flow without invented PCs (strict mode + ban-aware recovery) | Partial |
| P0 | Guest GCM PUT advances without boot assist | Blocked |
| P0 | Import / HLE returns that do not land on dead islands | Partial |
| P1 | Title-authored RSX draws and shaders | Blocked on P0 |
| P1 | SPU static lifter (not only interpreter stub) | Planned |
| P1 | Differential PPU/SPU tests | Planned |

Boot FIFO assist proves **FIFO → decode → present**. It does **not** prove title meshes.

## Stage architecture (target)

```
ELF/PRX analysis → CFG / OPD / imports
        ↓
PPU decode → semantics → IR → codegen
SPU decode → semantics → lifter → codegen
        ↓
Runtime (memory, kernel HLE, threads)
        ↓
RSX (command processor → state → shaders → backend)
        ↓
Native link + verification reports
```

Current tree still concentrates analysis/lift in `src/core` and runtime in `runtime/`. New paths under `src/analysis`, `src/ppu`, `src/spu`, `graphics/`, and `tests/conformance` are introduced **incrementally**; the public `ps3core` API stays stable for GUI/CLI.

## Subsystem policy

### PPU

- Separate **decode**, **semantic result**, and **codegen** over time.  
- No counting “emitted C++” as verified semantics.  
- Unsupported encodings must be explicit (not silent no-ops).  
- `PS3RT_STRICT_FAULT=1` for correctness runs; soft recovery is diagnostic only.

### SPU

- Keep interpreter/MFC for runtime.  
- Add a **dedicated static SPU pipeline** (decoder/lifter) before treating SPURS as complete.

### HLE

- Expand only from **observed** NIDs and syscalls in title logs.  
- Track Complete / Partial / Stub per module in docs and code metadata.

### RSX

- Split command processor, guest state, shaders, backends as files grow.  
- Guest draws require non-zero methods from **title** PUT, not only assist cmdstreams.

### Testing

- Unit tests for ELF bounds and instruction groups.  
- Differential tests for isolated PPU/SPU ops.  
- Per-game milestone matrix under `docs/games/` (load → lift → build → init → threads → SPU → graphics → playable).

## Explicit non-goals (for now)

- Claiming parity with any other recompiler by file count or NID count.  
- Shipping decryption or third-party emulator integration.  
- Treating assist/diagnostic geometry as game compatibility.
