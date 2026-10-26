# Phase XV — Real rendering

**Status:** ACTIVE (2026-10-26)

## Objective

Connect guest RSX work to host D3D11 so visible frames come from **game** command streams, not diagnostic host geometry.

## Subphases

| ID | Status | Description |
|----|--------|-------------|
| XV-A | DONE | Split `draws_guest` / `draws_diagnostic` metrics |
| XV-B | PARTIAL | FIFO packet count, multi-arg methods, JUMP (control-flow depth still thin) |
| XV-C | PARTIAL | Present: guest VA / color first; solid only if no guest draw |
| XV-D | PARTIAL | Indexed expand + multi-attribute; needs title validation |
| **XV-E** | **IN PROGRESS** | Honest VP/FP translation (fail closed); VMX residual → runtime dispatch |
| XV-F | NEXT | Title texture formats |
| XV-G | NEXT | Depth / blend / viewport for one stable scene |

## XV-E notes

- Unknown VMX forms call `ps3rt_vmx_dispatch` (lift counts translated; runtime implements class fallbacks)
- SCE VP/FP: known op set; fail-closed direction for empty translates

## Subphase depth audit (A–E)

| ID | Depth status | Gaps |
|----|--------------|------|
| XV-A | Adequate | metrics present |
| XV-B | Partial | JUMP/call/return full semantics still thin |
| XV-C | Partial | guest present path exists; titles still often diagnostic |
| XV-D | Partial | indexed + multi-attr in tree; needs title validation |
| XV-E | In progress | SCE VP/FP full ISA not done |

See [ROADMAP.md](../ROADMAP.md).
