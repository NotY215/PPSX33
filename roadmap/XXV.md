# Phase XXV — Truthful execution & first real pixels (ACTIVE)

**Last updated:** 2026-10-25

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **A–H** | DONE (untested) | Lifter, RSX host, faults, PRX/LV2, SPU/MFC | various |
| **I** | DONE (untested) | FS / audio / pad / save full service depth | `assets.cpp`, `audio_save.cpp`, `ps3rt_b.inl` |
| **J** | DONE (untested) | End-to-end service checklist | `ps3rt_b.inl` |
| **K** | PLANNED | Guest RSX geometry | `rsx_stub.cpp` |
| **L** | PLANNED | Boot stability (wild-pc / imports) | `ppu_runtime_run.h` |

### XXV-I depth

| Area | Work |
|------|------|
| cellFs | open/read/write/close/lseek/stat + mkdir/unlink/rename/fsync/GetFreeSize |
| Audio | waveOut keepalive buffers so output is not a single prove-tone |
| Save | create/list/load/delete + status/autosave NID family |
| Pad | GetData surface + poll each time slice |

### XXV-J

After GCM init, logs once: boot_mem, gcm, pad, flips, PPU/SPU slices, remaining K/L.

### Why GOW still has no real scene

Runtime log: `shaders=0`, black guest surfaces, repeated wild-pc recovery. That is **K** (geometry) and **L** (control flow), not I/J service stubs.

## Related

[ROADMAP.md](../ROADMAP.md) · [XXIV.md](XXIV.md)
