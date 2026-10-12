# PPSX33 Roadmap

**Last updated:** 2026-10-12 (Phase V)

| Phase | Status | Focus |
|-------|--------|-------|
| A–U | Done | Boot survival, sticky loops, SPU HLE |
| **V** | **Done** | Correct NV method packets; known-method decode; CLEAR=draw; NID candidates; inject guest FIFO |
| **W** | Next | Textures / real guest put from libgcm |

## Phase V

**Bug:** `methods=10 unknown=10` — packet format was wrong:
`hdr = (1<<18)|((method>>2)&0x7FF)` decoded to offset 0x74 instead of 0x1D8.

**Fix:**
- Encode: `hdr = (count<<18) | (method & 0x1FFC)`
- Decode: increasing + non-increasing method types
- CLEAR_SURFACE counts as draw; log `known=` vs `unknown=`
- Inject CLEAR/viewport into FIFO every 32 progress ticks
- Expanded NID candidate scan (8 sources)

## Apply

```bat
cd /d E:\PPSX33
git pull origin main
```

Expect:
```
[rsx] flip #N draws=N clears=N methods=... known=... unknown=0 ...
```
