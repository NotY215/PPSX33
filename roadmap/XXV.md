# Phase XXV — Truthful execution & first real pixels (ACTIVE)

**Last updated:** 2026-10-25

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **A–J** | DONE (untested) | Through services + E2E checklist | various |
| **K** | DONE (untested) | Guest color paint → first presentable frames | `rsx_stub.cpp` |
| **L** | DONE (untested) | Sticky wild-pc break + LR-first recovery | `ppu_runtime_run.h` |

### XXV-K (start frame rendering)

RSX path paints the guest color surface (gradient + triangle) on clear/draw/present while shaders are unbound, then uploads non-black pixels to D3D11. Goal is a **visible frame**, not stable FPS. Host solid triangle remains as fallback.

### XXV-L (boot control flow)

Wild-pc recovery no longer sticks on one alternate PC. After six identical alts: try LR, then +0x10, then next translated chunk. `recover_return_pc` prefers LR when it is in a translated chunk.

### Follow-on (not K/L)

Full NV40 vertex/fragment program translation and title-driven FIFO geometry remain next once a painted frame is confirmed on screen.

## Related

[ROADMAP.md](../ROADMAP.md) · [XXIV.md](XXIV.md)
