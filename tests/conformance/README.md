# Conformance tests

Goal: prove instruction and subsystem semantics without relying on title recovery loops.

## Planned layout

| Path | Purpose |
| --- | --- |
| `ppu/` | Isolated PPU op tests (GPR/CR/XER/memory) |
| `spu/` | SPU lane/LS/channel/DMA tests |
| `runtime/` | Thread, FS, import dispatch smoke tests |
| `graphics/` | FIFO packet → state → backend (null + D3D11) |

## Policy

- Prefer `PS3RT_STRICT_FAULT=1` for runtime tests.
- A test that only “opens a window” does not pass graphics.
- Failures must save seed, input state, and generated artifacts.

## Current

Scaffolding only. Add executables as the harness lands.
