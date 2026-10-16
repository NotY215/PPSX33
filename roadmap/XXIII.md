# Phase XXIII — Execution reality (ACTIVE)

**I–Z surfaces in tree; full end-to-end title render still depends on guest FIFO + testing.**

## Subphases

| ID | Status | Focus |
|----|--------|-------|
| **A–W** | DONE (untested) | Prior surface work including HLSL blit T and host draw V |
| **X** | DONE (untested) | Diagnostic / force visible frame path each present |
| **Y** | DONE (untested) | HLSL sample path retained (full RSX microcode ISA still iterative) |
| **Z** | DONE (untested) | Pitch align 64B; color/tex present path |

## GUI

Build-complete MessageBox string fixed (no raw newlines / en-dash in constant).

## Frame goal

Logs should show `[rsx] XXIII-X diagnostic frame` and/or `[rsx] XXIII-T/V HLSL textured quad draw` when the window is up. Real game pixels still need guest PUT and memory filled by the title.
