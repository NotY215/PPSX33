# Phase XXV — Truthful execution & first real pixels (ACTIVE)

Cross-checked against the GPT implementation audit and the current tree / GOW logs.
FPS is not a goal until guest frames are correct.

**Last code packs:** `PPSX33_XXV_E_render_lifter.zip` then `PPSX33_XXV_D_full_depth_on_E.zip` (D applied on E).

---

## Audit acceptance

| Audit claim | Verdict | Response in XXV |
|-------------|---------|-----------------|
| Silent `return true` in `ppu_lifter.cpp` | **Accepted** | **XXV-C/E** — unsupported primary/xo31 return false + report keys |
| Wild-PC recovery masks faults | **Accepted** | **XXV-F** planned |
| RSX not a faithful renderer | **Accepted** | **XXV-D** host pipeline depth (not full shader ISA) |
| LV2/HLE success stubs | **Accepted** | **XXV-G** planned |
| SPU/MFC/SPURS approximate | **Accepted** | **XXV-H** planned |
| Full RSX shader ISA in one drop | **Deferred** | D delivers FIFO/surfaces/host draw; VP/FP translate later |

---

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXV-A** | DONE (untested) | Audio `audio_queue_s16` C3861; fail-once open | `audio_save.cpp` |
| **XXV-B** | DONE (untested) | Visible host present path | `rsx_stub.cpp` |
| **XXV-C** | DONE (untested) | Empty primary opc not Implemented | `ppu_lifter.cpp` |
| **XXV-D** | DONE (untested) | Full depth part1+2 on E | `rsx_stub.cpp`, `ps3rt_b.inl` |
| **XXV-E** | DONE (untested) | Lifter honesty + present order + solid triangle | `ppu_lifter.cpp`, `rsx_stub.cpp` |
| **XXV-F** | PLANNED | Strict faults: validate_guest_pc, capture_guest_fault | `ppu_runtime_run.h` |
| **XXV-G** | PLANNED | PRX/LV2 contracts | `ps3rt_b.inl`, `lv2_syscalls.inl` |
| **XXV-H** | PLANNED | SPU/MFC/SPURS layers | `spu_stub.cpp` |
| **XXV-I** | PLANNED | FS / audio / pad / save | assets, audio, pad |
| **XXV-J** | PLANNED | End-to-end title checklist | docs + logs |

### XXV-D full depth (landed in pack)

**Part 1**
- `rsx_state_apply_method` — color/pitch/clear/tex/viewport/scissor/shader/draw methods
- `rsx_fifo_decode_word` — FIFO words → apply; draw methods → `rsx_draw_host`
- `rsx_sync_guest_host` / `rsx_draw_host` — guest surface upload + HLSL or solid fallback
- Present: **clear → draw → Present** (no wipe-after-draw)
- `ps3rt_rsx_set_color_surface` + GCM seed pattern when PUT stuck

**Part 2**
- `rsx_state_apply_method_p2` — depth/blend/color mask, Z offset/pitch, vertex array slots
- `rsx_vertex_fetch_note` — records offsets/strides (not full geometry yet)
- `rsx_apply_viewport_scissor` on host draws

**Limit:** Host D3D11 path + optional guest buffer blit. Not full RSX VP/FP translation or indexed game meshes.

### XXV-E (landed in pack)

- Unknown primary / xo31 → `return false` + missing-key report line
- Present order fix; solid-color triangle GPU path
- Apply **before** D full-depth pack when patching from releases

### XXV-F — Deterministic execution faults (next)

- `validate_guest_pc`, `resolve_guest_branch`, `resolve_function_descriptor`, `resolve_import`
- `capture_guest_fault`, `trace_guest_callstack`
- Strict mode: fault dump instead of endless wild-PC recovery

---

## Log signals

| Milestone | Expect |
|-----------|--------|
| Host present | `[rsx] XXV-E solid triangle` or guest/tex present lines |
| D depth FIFO | `[rsx] XXV-D draw method=…` when methods fire |
| Seeded surface | `[gcm] XXV-D-depth seeded color pattern` |
| Honest lift | `unimplemented` / missing encodings in `lift_report` |
| Real game frame | Guest PUT advances; methods/shaders/tex from title — still open |

---

## Related

[ROADMAP.md](../ROADMAP.md) · [XXIII.md](XXIII.md) · [XXIV.md](XXIV.md)
