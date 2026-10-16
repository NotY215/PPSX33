# Phase XXIV — Boot-to-first-pixels (ACTIVE)

Log `2026-10-16_14-31-14`: audio spam fixed; HLSL ready; diagnostic uploads at `0xf000000`; PRX still `mod=?`; no game COLOR_OFFSET.

## Subphases

| ID | Status | Focus | Files |
|----|--------|-------|-------|
| **XXIV-A** | DONE | Audio fail-once | `audio_save.cpp` |
| **XXIV-B** | DONE | FIFO bound / method counts | `rsx_stub.cpp`, `ps3rt_b.inl` |
| **XXIV-C** | DONE | HLSL prove + diagnostic | `rsx_stub.cpp` |
| **XXIV-D** | DONE (untested) | PRX NID filter (reject 0x48xxxxxx); better candidates; lr log | `ps3rt_b.inl` |
| **XXIV-E** | DONE (untested) | Always Present; prefer COLOR_OFFSET; brighter diagnostic; prove-tone | `rsx_stub.cpp`, `audio_save.cpp` |

## Expected after D–E

- `[ps3] XXIV-D prx_import …` with fewer garbage NIDs
- `[rsx] XXIV-E COLOR_OFFSET=…` when guest sets surface
- Bright bar diagnostic if no color buffer
- `[audio] XXIV prove-tone` once when waveOut opens
- Present every flip

## Still needed for game pixels / game audio

- Guest PUT advancing with real methods (boot past imports)
- cellAudio feeding PCM (prove-tone only validates host path)
