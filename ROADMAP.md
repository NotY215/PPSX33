# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A | Done | UI, project-named exe, progress/ETA |
| B | Done | Halt recovery (sticky last_good, wide stack) |
| C | Done | LV2 expansion + PRX/NID filter |
| D | Done | Never hard-halt while last_good exists |
| E | In progress | Break null-call loops + restore TOC + real GCM state |
| F | Pending | Import table / OPD resolution |
| G | Later | SPU bring-up |
| H | Later | RSX command-buffer drain |

## Phase E (this push)

- When the same LR is used for a null-call >4096 times, try an *alternate* return from the stack (exclude looping LR).
- Soft-break advances +4 / +8 / +16.
- After 8 soft-breaks on the same site, clear sticky last_good so we stop spinning.
- Restore r2 (TOC) from entry TOC if the guest zeroes it.
- Escape limit lowered to 262144 to fail faster with useful logs.
- Halt line now includes soft_breaks=.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Then rebuild (GUI Build) and run GOW_3 again.

Look for:

```
[ps3] null-loop break: lr=0x199c78 -> alt=0x……
[ps3] null-loop break: lr=0x199c78 +4
[ps3] heartbeat … r2=0x52d6c8
```
