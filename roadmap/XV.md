# Phase XV — Real rendering

## Screenshots

1. **Colorful triangle + bars** — inject + `paint_guest_color_buffer` working (seed frame).
2. **Black at high flip** — often swap-chain resize (720→729) or clear/blit of empty guest color.

## XV-H (this pack)

- Harden resize + RTV recreate (prevent black Present)
- Never pure-black clear
- Every idle frame: repaint guest bars+triangle → blit → NV40 on top
- Host NDC seed triangle if VA read fails
- Solid diagnostic only as last resort

## Next (real title geometry)

Game must advance GCM PUT past GET with real vertex arrays (not inject-only).
