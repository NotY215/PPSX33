# Historical Phase F runtime note

This file records the runtime split introduced during Phase F. It is retained as development history, not as the current roadmap or recovery procedure.

The current PPU runtime layout is:

- `ppu_runtime.h`: include wrapper.
- `ppu_runtime_core.h`, `ppu_runtime_core_a.h`, and `ppu_runtime_core_b.h`: shared runtime definitions and helpers.
- `ppu_runtime_run.h`: PPU dispatch and recovery loop.

Phase G is now recorded as complete in [the repository roadmap](../ROADMAP.md), including guest `cellGcmSys` state and additional PPU return-recovery logic. Phase H is the next milestone. Keep the split headers together and use the current build scripts rather than restoring an older header from history without reviewing the consequences.
