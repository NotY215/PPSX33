# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A–G | Done | Boot → GCM state |
| H | Done | Spin-loop detection + progress_hw |
| **I** | **Build fix done** | Remove duplicate SPUContext; safe SPU load |
| J | Next | Deeper RSX method decode / real draws |

## Phase I (this turn)

- Removed duplicate `struct SPUContext` from `ppu_runtime_core_b.h` (kept in core_a)
- `ps3rt_spu_load`: use explicit size ternary instead of `std::min`
- `ps3rt_progress_hw` retained for wait-loop progress

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build game.

Build should succeed. Run and look for:
```
[gcm] control @ ...
[ps3] Phase H spin ...
[rsx] flip #...
```
