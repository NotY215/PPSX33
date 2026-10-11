# PPSX33 Roadmap

Last updated: 2026-10-11 (Phase N)

| Phase | Status | Goal |
|-------|--------|------|
| A–G | Done | Boot, GCM, RSX D3D11 |
| H–M | Done | Spin breaks, mem-touch, soft skips |
| **N** | **Done** | Break last_good ping-pong; harden null recovery |
| **O** | Next | Real NID from TOC/OPD + import table |
| P | Planned | RSX method decode / draw |
| Q | Planned | SPU scheduling |

## Phase N fix

**Bug:** `escape -> last_good=0x336588` then null-loop on that same address reset soft_breaks and escaped to the **same** last_good forever (app hang).

**Fix:**
1. Track `failed_escape` — never re-escape to a target that failed once
2. After 2 failed escapes, **ban** last_good and clear it
3. Prefer stack recover over last_good
4. Only set last_good from non-null, non-import returns
5. Ban LR on every null sticky hit
6. Hard-escape / halt if soft_breaks > 10 with no viable target

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect:
```
[ps3] Phase N ban=0x336588
[ps3] Phase N stack-escape -> 0x...
[ps3] heartbeat ...
```
Not a repeating `escape -> last_good=0x336588` loop.
