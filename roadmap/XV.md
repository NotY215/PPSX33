# Phase XV — Real rendering

**Status:** ACTIVE (2026-10-26)

## Objective

Connect guest RSX work to host D3D11 so visible frames come from **game** (or valid GCM) command streams, not diagnostic host geometry.

## Subphases

| ID | Status | Description |
|----|--------|-------------|
| XV-A | DONE | Split `draws_guest` / `draws_diagnostic` metrics |
| XV-B | IN TREE | FIFO packet count, multi-arg methods, JUMP |
| XV-C | IN TREE | Present: guest VA / color first; solid triangle only if no guest draw this frame |
| XV-D | NEXT | Index buffer Draw path + multi-slot attributes |
| XV-E | NEXT | Honest VP/FP translation (fail closed) |
| XV-F | NEXT | Title texture formats |
| XV-G | NEXT | Depth / blend / viewport for one stable scene |

## Non-goals (this phase)

- Full game playability  
- Perfect NV40 shader ISA  
- OpenGL backend (later)

## Related prior work (folded into I–XIV)

Former labels XXIII–XXVI (execution reality, boot pixels, UI) map into phases IX–XIV.
