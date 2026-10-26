# Phase XV — Real rendering

**Status:** ACTIVE (2026-10-26)

## Objective

Connect guest RSX work to host D3D11 so visible frames come from **game** command streams, not diagnostic host geometry.

## Subphases

| ID | Status | Description |
|----|--------|-------------|
| XV-A | DONE | Split `draws_guest` / `draws_diagnostic` metrics |
| **XV-B** | **DEEPENED** | FIFO JUMP / CALL / RETURN control-flow walk |
| XV-C | PARTIAL | Present: guest VA / color first; solid only if no guest draw |
| XV-D | PARTIAL | Indexed expand + multi-attribute; host IB path (`g_dev` fix) |
| **XV-E** | **IN PROGRESS** | SCE VP/FP + VMX residual dispatch |
| XV-F | NEXT | Title texture formats |
| XV-G | NEXT | Depth / blend / viewport for one stable scene |

## Build fix

- `g_dev11` → `g_dev` (`ID3D11Device*`) in indexed IB create path

## Controller

- Glow restored on active pad parts
- Modifier capture uses `e.code` + `e.location` (Shift/Ctrl/Alt Left|Right)
- Host map round-trip: `LShiftKey` ↔ `ShiftLeft`, etc.

See [ROADMAP.md](../ROADMAP.md).
