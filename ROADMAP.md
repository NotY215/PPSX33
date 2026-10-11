# PPSX33 Roadmap

Last updated: 2026-10-11 (Phase P)

| Phase | Status | Goal |
|-------|--------|------|
| A–O | Done | Boot, stub loops, TOC, escapes |
| **P** | **Done** | RSX flush order, LV2 wait HLE, reject 0x10000 escapes |
| **Q** | Next | Real GCM command emission + method counts > 0 |
| R | Planned | Draw path / shaders |

## Phase P

**Progress from O:** Real syscalls, TOC stable, 5000+ flips, heartbeat at real PCs.

**Still wrong:**
- `draws=0 methods=0` — `progress_hw` synced get=put *before* flush (FIFO never decoded)
- Stack-escape to `0x10000` (image base) is useless
- Busy `sc 141` usleep / `sc 144` time / mutex locks need HLE

**Fixes:**
1. Flush FIFO **before** setting guest get=put
2. cellGcmSys stubs advance PUT and write a NOP method into the FIFO
3. `recover_return_pc` rejects targets `< 0x11000`
4. New `runtime/lv2_syscalls.inl` for wait/time/mutex syscalls

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Rebuild All → Build → Run.

Expect:
```
[rsx] flip #N draws=... methods=...   # methods should climb
[lv2] sc 141 ...   # returns immediately
```
No more `stack-escape -> 0x10000`.
