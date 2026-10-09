# PPC64 sample games

`pong/` contains a deterministic text-mode Pong sample intended to exercise the loader and lifter with a small, repeatable program. It is a pipeline test, not a graphical PS3 game. Graphical output requires working RSX/GCM support, which is not currently established.

Build it with a compatible PPC64 big-endian PS3 toolchain. The exact ELF entry point and OPD layout are SDK/toolchain-specific; preserve those conventions when packaging the sample ELF for PPSX33. See the root README and PPU coverage report for the current status.