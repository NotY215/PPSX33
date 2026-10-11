# PPSX33 Roadmap (2026-10-11)

| Phase | Status | Goal |
|-------|--------|------|
| A | Done | UI, project-named exe, progress/ETA |
| B | Done | Halt recovery (sticky last_good, wide stack) |
| C | Done | LV2 expansion + PRX/NID filter |
| D | Done | Never hard-halt while last_good exists |
| E | Done | Break null-call loops + restore TOC |
| F | Done | Null-table skip + header copy |
| **G** | **Done** | Real cellGcmSys state (ctrl/labels/flip) + LR ban |
| H | Next | SPU execution and RSX command draining |

## Phase G (completed)

**ps3rt.cpp**
- `gcm_ensure_state()` allocates guest CellGcmControl + label area
- Handlers: GetControlRegister, GetLabelAddress, InitBody, SetFlip, GetFlipStatus, MapMainMemory, …
- `looks_like_nid()` rejects pointer-like garbage from r11
- Any kind=1 (cellGcmSys) call ensures GCM state exists

**ppu_runtime_run.h**
- Permanent ban list (up to 8 LRs) after soft_breaks > 48
- Deeper stack scan (±0x400 / +0x2000)
- Force-skip / alt-return when region exhausted

## Phase H (next)

Focus the next milestone on advancing SPU execution and draining RSX commands through the existing runtime interfaces. Keep partial implementations explicitly labeled, and add reproducible tests/logs for any newly supported command or instruction. Do not describe this milestone as complete until the relevant code paths and validation are present.

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Then **Rebuild All**, then **Build** the game.

### Look for
```
[gcm] control @ 0x...
[gcm] labels  @ 0x...
[gcm] Phase G state ready fifo=0x... ctrl=0x...
[ps3] Phase G ban LR=0x33723c (nban=1)
[ps3] null-loop escape via alt=0x...
```
