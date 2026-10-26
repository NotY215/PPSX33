# Phase XV — Real rendering

## Log diagnosis (19:51)

- `put=get` — game FIFO idle
- Inject draws 3 verts then fullscreen clear-color blit covers them
- Depth/texture bind could reject or recolor geometry

## XV-G

- Viewport + scissor full window
- D3D11 depth/blend/rasterizer states; depth off for early draws
- NV40 vertex colors; ignore near-clear textures
- Present: geometry after clear, no covering blit when VA draws
- Inject: larger triangle, depth/blend off methods

## Honest status

Inject triangle is a **seed** until title advances PUT/GET with real GCM.
