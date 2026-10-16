# Phase XXV — Truthful execution & first real pixels (ACTIVE)

Cross-checked against the GPT implementation audit and the current tree / GOW logs.
FPS is not a goal until guest frames are correct.

---

## Audit acceptance

| Audit claim | Verdict | Response in XXV |
|-------------|---------|-----------------|
| Silent `return true` in `ppu_lifter.cpp` | **Accepted** | XXV-C started; XXV-E completes LiftStatus-style honesty |
| Wild-PC recovery masks faults | **Accepted** | XXV-F strict fault mode + capture |
| RSX not a faithful renderer | **Accepted** | XXV-D pipeline skeleton → real draws |
| LV2/HLE success stubs | **Accepted** | XXV-G contracts, no silent success for unknown NID |
| SPU/MFC/SPURS approximate | **Accepted** | XXV-H layered semantics |
| Full RSX shader ISA in one drop | **Deferred** | XXV-D starts FIFO/surfaces/synthetic; full ISA later |
| Drop all recovery immediately | **Partial** | Strict default optional; recovery remains explicit diagnostic mode |

---

## Subphases (detailed)

### XXV-A — Build / audio host (DONE untested)
- **Focus:** `audio_queue_s16` forward declare (C3861); host waveOut path stays fail-once.
- **Files:** `runtime/audio_save.cpp`
- **Approach:** Host open proof separate from guest cellAudio PCM feeding.

### XXV-B — Visible host present (DONE untested)
- **Focus:** Every flip presents; bright clear + `[rsx] XXV-FRAME` log when guest content absent.
- **Files:** `runtime/rsx_stub.cpp`
- **Limit:** Proves D3D11 present path, not game geometry.

### XXV-C — Lifter honesty (partial)
- **Focus:** Empty primary opc cases must not report Implemented.
- **Files:** `src/core/ppu_lifter.cpp`
- **Next (E):** explicit status, unsupported report file.

### XXV-D — RSX real pipeline start (IN PROGRESS)
**Goal:** Decode real FIFO methods; bind guest color/depth; present **guest** surface when valid; synthetic triangle only as unit check.

Recommended functions (adapt names to tree):
- `rsx_fifo_decode()` — method headers, lengths, jump/call/return, bounds
- `rsx_state_apply_method()` — register state from methods
- `rsx_surface_bind()` — color/depth offsets, pitch, format
- `rsx_texture_decode()` / upload — formats, pitch
- `rsx_draw()` — primitive execution (host path)
- `rsx_sync_guest_host()` — surface coherence
- `rsx_present()` — final flip of **game** RT when available

Order inside D:
1. FIFO GET/PUT consistency (no fake method floods)
2. COLOR_OFFSET / pitch / format tracking
3. Present guest color buffer every flip when `color_valid`
4. Minimal non-indexed colored triangle path (synthetic)
5. Only then attempt guest draw methods with vertex fetch stubs

**Files:** `runtime/rsx_stub.cpp`, `runtime/ps3rt_b.inl` (GCM assist)

### XXV-E — PPU translation truthful (PLANNED)
- Separate decode vs semantic emission.
- Prefer status: `Implemented` / `Unsupported` / `InvalidEncoding` (map onto existing `lift_one` carefully).
- Every supported opcode emits real ops or documented architectural no-op.
- Unsupported → diagnostic failure, not success.
- Emit report of encodings that hit fallback.
- Audit: branches (bc/bclr/bcctr), CR/Rc, CA/OV, BE memory, FP edge cases, VMX lanes, lwarx/stwcx.

**Files:** `src/core/ppu_lifter.cpp`

### XXV-F — Deterministic execution faults (PLANNED)
Functions / behaviours:
- `validate_guest_pc(pc)` — translated / guest mem / import / invalid
- `resolve_guest_branch(target)` — no silent retarget
- `resolve_function_descriptor(addr)` — entry + TOC
- `resolve_import(module, nid)` — only when known
- `capture_guest_fault(ctx, reason)` — PC/LR/CTR/CR/GPR/SP/TOC + branch history
- `trace_guest_callstack(ctx)`

Default: detailed fault on unknown destination. Recovery only in explicit diagnostic mode; log every recovery.

**Files:** `runtime/ppu_runtime_run.h`

### XXV-G — PRX / LV2 contracts (PLANNED)
- Explicit module metadata; resolved-import table (module, NID, status, call count).
- Unknown NID: documented error or strict stop — **not** automatic success.
- HLE helpers shaped like contracts (mutex/cond/event/memory/fs) with real state where boot requires it.
- Document NID, args, memory effects, errors, blocking, tests.

**Files:** `runtime/ps3rt_b.inl`, `runtime/lv2_syscalls.inl`, `runtime/nid_table.h`, `src/core/elf_loader.cpp`

### XXV-H — SPU / MFC / SPURS (PLANNED)
Layers: instruction semantics → channels/mailboxes → MFC list/tags/ordering → SPURS dispatch → PPU↔SPU visibility.

Helpers: `spu_execute_one`, `mfc_submit_command`, `mfc_advance_transfers`, `mfc_tag_complete`, `spurs_dispatch_ready_jobs`.

**Files:** `runtime/spu_stub.cpp`

### XXV-I — Game services (PLANNED)
- cellFs: fail hard on bad decompress; bounds checks
- cellAudio: guest PCM → host (separate from prove-tone)
- Pad: native semantics vs keyboard map
- Save data API contracts

### XXV-J — End-to-end proof (PLANNED)
Boot → menu → audio → input → scene → save/load checklist per title; structured logs.

---

## Implementation loop (mandatory)

1. Capture failing PC / opcode / NID / RSX method / SPU op
2. Define expected semantics (docs / reference)
3. Minimal regression where possible
4. Real behaviour — no silent success
5. Re-run title log + unit checks

---

## Log signals (success criteria)

| Milestone | Log / visual |
|-----------|----------------|
| Host present alive | `[rsx] XXV-FRAME present` or guest color blit |
| Guest surface | `COLOR_OFFSET` set + non-diagnostic tex |
| Honest lift | unsupported encodings reported, not counted as translated |
| Strict fault | fault dump instead of endless wild-pc to same addr |
| Real frame | methods/shaders/tex reflect guest stream; not only inject |

---

## Related

[ROADMAP.md](../ROADMAP.md) · [XXIII.md](XXIII.md) · [XXIV.md](XXIV.md)
