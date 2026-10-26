# Phase XV — Real rendering

**Status:** ACTIVE (2026-10-26)

## Objective

Connect guest RSX work to host D3D11 so visible frames come from **game** command streams, not diagnostic host geometry.

## Subphases

| ID | Status | Description |
|----|--------|-------------|
| XV-A | DONE | Split `draws_guest` / `draws_diagnostic` metrics |
| XV-B | DONE | FIFO packet count, multi-arg methods, JUMP |
| XV-C | DONE | Present: guest VA / color first; solid only if no guest draw |
| XV-D | DONE | Indexed expand + multi-attribute (VA0 pos, VA1 varying); Draw/DrawIndexed |
| **XV-E** | **IN PROGRESS** | Honest VP/FP translation (fail closed); PPU AltiVec + SPU RR depth |
| XV-F | NEXT | Title texture formats |
| XV-G | NEXT | Depth / blend / viewport for one stable scene |

## XV-E notes (depth)

- SCE VP/FP: known op set only; unknown ops do not count as successful translation (fail-closed direction)
- PPU: IBM AltiVec coverage includes `vspltis*`, unpack, min/max, avg, logical, rotate forms
- SPU: additional RR mul/div/cmp/shift/float + stop
- GOW3 lift baseline: instructions 1285560, translated 1254880, unimplemented 30680 (~2.4%)

See [ROADMAP.md](../ROADMAP.md).
