# PS3 PPU/RSX Sample

`pong/pong.c` is a PlayStation 3-targeted C sample, not a native desktop game. It uses PSL1GHT headers and RSX/GCM display-buffer APIs, draws a simple Pong scene into a framebuffer, reads PS3 controller input, and includes a VMX/AltiVec operation probe.

The sample is designed to create a PS3 PPU ELF for the PPSX33 pipeline. The intended test path is:

1. Compile `pong/pong.c` with a compatible PSL1GHT `ppu-gcc` toolchain.
2. Produce `build/pong/pong.elf`.
3. Run the ELF through `ps3_cli` to create a project, lift PPU instructions, and invoke the native build.
4. Inspect the lift report and generated output. A successful native build does not mean the PS3 graphics APIs or gameplay will work in the host runtime.

From the repository root, the helper scripts are `scripts/build_pong_recompile.bat` and `scripts/build_pong_recompile.sh`. They require the PS3 homebrew SDK and a previously built PPSX33 CLI. This pipeline is experimental and must not be described as passing unless it has actually been run and verified.

The source directory intentionally contains only the target-specific `pong.c` sample. Do not add copyrighted commercial-game assets or binaries.
