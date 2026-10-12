# PPSX33 Roadmap

**Last updated:** 2026-10-12 (Phase U)

| Phase | Status | Focus |
|-------|--------|-------|
| A–T | Done | Boot, GCM, FIFO modulo, sticky LR bans |
| **U** | **Done** | Region-ban set + hard-leave; no re-leave to same PC; SPU complete path |
| **V** | Next | Real NID + draw methods |

## Phase U

**Bug:** `leave sticky -> 0x3fe11c` + `ban-region nban=32` repeated forever.
Ban table was full; leave target was always the same; region was re-entered.

**Fix:**
1. Separate **region ban set** (0x400 windows) from LR ban list
2. Circular buffers — never stuck at nban=32
3. If PC is inside a banned region → force-out immediately
4. Never leave to the same `last_leave` twice; hard-leave to another chunk
5. Wider stack scan + chunk-start fallback

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Expect: `Phase U leave sticky` / `hard-leave` at most a few times, then heartbeats on varying PCs — not the same leave target forever.
