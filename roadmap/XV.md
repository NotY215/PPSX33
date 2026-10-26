# Phase XV - Real rendering

## Log 21:25 (before XV-K)
- `get=0x803c8` vs `put=0x3c8` (GET far ahead)
- `methods~131072` / `packets~65536` every flip (FIFO walk cap from false JUMP)
- Cause: JUMP matched `(hdr & 3)==1` on normal method headers

## XV-K
- JUMP only on `(hdr & 0xE0000003)==0x20000000`
- CALL only on proper encoding; targets mod fifo_size
- PUT/GET always mod fifo_size; resync if ring distance > half
- Tighter process cap (0x800)
- Inject throttled until real stream heuristic
- Skip UNKNOWN log for nid 0

## Still not game geometry
Seed remains until title authors a real GCM stream after boot.
