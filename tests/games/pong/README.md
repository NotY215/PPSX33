# PPSX33 Pong test ELF

This sample is a small PowerPC64 big-endian program intended to exercise the ELF loader and generated-code pipeline before attempting a graphical game. It is a deterministic text-mode Pong simulation, not an RSX-rendered PS3 game. It can be built into a PS3-format ELF only with a compatible PPC64 big-endian toolchain and the project's expected entry/OPD conventions.

## Build requirements

- A PS3 homebrew toolchain that targets 64-bit big-endian PowerPC
- The corresponding SDK headers and linker setup
- A build environment capable of producing an ELF64 big-endian executable

## Status

The sample source is provided as a starting point for testing control flow, integer arithmetic, loops, comparisons, and data access. It does not claim VMX opcode coverage or graphical RSX support. Keep the generated ELF and lift report together when reporting failures.