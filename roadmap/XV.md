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
| **XV-D** | **IN TREE** | Indexed expand + multi-attribute (VA0 pos, VA1 varying); Draw/DrawIndexed |
| **XV-E** | **IN PROGRESS** | Honest VP/FP translation (fail closed); PPU AltiVec + SPU RR depth |
| XV-F | NEXT | Title texture formats |
| XV-G | NEXT | Depth / blend / viewport for one stable scene |

## XV-D notes

- Index arrays (16/32-bit) resolve vertex indices before host VB fill
- Second vertex array slot feeds a generic varying
- Logs: `[rsx] XV-D draw verts=… idx=yes|no va0=… va1=…`

See [ROADMAP.md](../ROADMAP.md).


## XV-E notes (depth)

- SCE VP/FP: known op set only; unknown ops do not count as successful translation
- PPU: additional IBM AltiVec (`vspltis*`, `vupk*`, `vmax*`/`vmin*`, `vavg*`, `vsel`/`vperm`/`vsldoi`, `vandc`/`vnor`/`vxor`/`vor`)
- SPU: mul/div/cmp/shift/float RR forms + stop
- GOW3 lift baseline before this pack: translated 1254880 / unimplemented 30680 (~2.4%)
