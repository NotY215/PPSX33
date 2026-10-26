# Phase XV — Real rendering

## Log diagnosis (GOW III 2026-10-26)

- `methods=0` / `shaders=0` / `tex=0` while `guest` counter rose every flip
- Root cause: present path treated synthetic/empty VA draws as **guest**
- Only ~9 FIFO packets/frame — game not yet driving a full GCM stream (HLE/import loop)

## Subphases

| ID | Status | Work |
|----|--------|------|
| A–B | DONE/DEEPENED | metrics, JUMP/CALL/RETURN |
| C | DEEPENED | color/tex blit; diagnostic only if no real work |
| D | DEEPENED | VA draw only when `methods_this_frame>0` and VA0 bound |
| E | DEEPENED | VP/FP + VMX dispatch |
| **F** | **DEEPENED** | CELL_GCM texture formats (B8, 16-bit, A8R8G8B8, DXT family fallback) |
| G | NEXT | depth/blend/viewport stability |

## UI

- Logo ring: double-buffered panel, 20fps, partial invalidate, cached fonts/pens
