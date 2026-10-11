# PPSX33 Roadmap

Last updated: 2026-10-11 (Phase O)

| Phase | Status | Goal |
|-------|--------|------|
| A–N | Done | Boot through null-region / last_good fixes |
| **O** | **Done** | Break import stub ping-pong + TOC restore |
| **P** | Next | Real NID from TOC/OPD + richer cellGcmSys HLE |
| Q | Planned | RSX method decode / draws |
| R | Planned | SPU scheduling |

## Phase O

**Symptom:** Heartbeats stuck at `pc=0x39800000 lr=0x336660` for 1M+ escapes until halt.
Code at `0x336660` immediately re-enters the import stub → infinite loop.

**Fixes:**
1. Detect consecutive same `(stub, lr)` pairs; after 4 → force `pc = lr+4` (or stack alt) and ban `lr`
2. On heartbeat, if PC still in import range → same forced break
3. Restore TOC (`r2`) when it looks corrupted (`> 0x10000000` or unaligned)
4. Raise escape ceiling; import returns no longer soft-loop forever

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect:
```
[ps3] Phase O break stub-loop lr=0x336660 count=4 -> 0x...
[ps3] Phase O hb-break stub -> 0x...
[ps3] heartbeat ... pc=0x33....   (not stuck on 0x39800000)
```
