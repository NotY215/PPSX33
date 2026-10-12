# PPSX33 Roadmap

Last updated: 2026-10-12 (Phase Q+R)

| Phase | Status | Goal |
|-------|--------|------|
| A–P | Done | Boot through RSX flush-order fix |
| **Q** | **Done** | Fix early-boot ban regression (`lr=0x103ac`) |
| **R** | **Done** | Import last-resort + protect early PC bans |
| **S** | Next | Real GCM draw methods / non-zero draws |

## Critical regression (Phase P)

`is_banned(p)` used `p < 0x11000`, which blocked **valid** early returns:
- entry `0x10230`, first import return `lr=0x103ac`
- Result: immediate halt at `pc=0x39800000 escapes=0`

## Phase Q+R fixes

1. Only ban `p < 0x10000` or `p == 0x10000` (raw segment base)
2. Never permanently ban early-boot LRs in `0x10004..0x1FFFF`
3. Import last-resort: if no ret found, still return to `lr` when aligned and `>= 0x10004`
4. Prefer `lr` for first cellGcmSys stubs during boot

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect again:
```
[gcm] control @ ...
[ps3] external/import stub pc=0x39800000 -> ret=0x103ac (#1)
... continues past boot ...
```
Not an immediate halt with `escapes=0`.
