# Phase XXV — Truthful execution & first real pixels (ACTIVE)

**Last updated:** 2026-10-25

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **A–E** | DONE (untested) | Audio, present, lifter honesty, RSX host depth | `audio_save.cpp`, `rsx_stub.cpp`, `ppu_lifter.cpp` |
| **F** | DONE (untested) | Deterministic faults / recovery visibility | `ppu_runtime_run.h` |
| **G** | DONE (untested) | PRX/LV2 contracts; strict import/LV2 | `ps3rt_b.inl`, `lv2_syscalls.inl` |
| **H** | DONE (untested) | SPU/MFC/SPURS depth; RSX C3861 fix | `spu_stub.cpp`, `rsx_stub.cpp`, `ppu_lifter.cpp` |
| **I** | PLANNED | FS / audio / pad / save services | |
| **J** | PLANNED | End-to-end title checklist | |

### XXV-H depth

| Area | Work |
|------|------|
| RSX | Forward decls for `rsx_apply_viewport_scissor`, `rsx_present_guest_or_diag`, `rsx_state_apply_method_p2` (fixes C3861) |
| SPU | Extra word add/sub/and/or/xor/shift/compare before unknown fallback |
| MFC | `mfc_complete_tag` on DMA paths |
| SPURS | WaitForWorkload contract log |
| PPU | Zero-padding lift; isel when applicable |

### Optional env

| Variable | Effect |
|----------|--------|
| `PS3RT_STRICT_FAULT=1` | Invalid PC faults (F) |
| `PS3RT_STRICT_IMPORT=1` | Unknown NID → CELL_ENOSYS (G) |
| `PS3RT_STRICT_LV2=1` | Selected LV2 → ENOSYS (G) |

## Related

[ROADMAP.md](../ROADMAP.md) · [XXIV.md](XXIV.md)
