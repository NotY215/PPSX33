# PPSX33 Roadmap — God of War 3 (GOW_3)

**Last updated:** 2026-10-12 (Phase T)

## Master phase table

| Phase | Status | Focus |
|-------|--------|-------|
| A–O | Done | Boot survival through stub ping-pong |
| P | Partial | RSX flush order (had early-ban regression) |
| Q–R | Done | Early-boot ban fix |
| S | Done | GCM method emit (methods > 0) |
| **T** | **Done** | FIFO put/get modulo (stop method storm); sticky-region bans |
| **U** | Next | Real NID recovery + recognize known methods |
| V–W | Planned | Textures + draws |
| X–Z | Planned | Shaders, SPU, audio |

## Phase T fixes (this commit)

**Method storm:** `put` grew without modulo while `get` used `% size` → `get != put` forever → millions of unknown methods.

- `rsx_process_fifo`: both put and get `% size`; step cap 4096; count clamp 32
- `ps3rt_rsx_set_put`: always `% size`
- Reject stack targets `< 0x10100` (kills `0x10010` escapes)
- After 6 hits on same 0x100-aligned region (e.g. `0x331c30`), ban the whole window and leave

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Expect: `methods=` stays modest (hundreds–low thousands), not tens of millions; no `stack-escape -> 0x10010`.
