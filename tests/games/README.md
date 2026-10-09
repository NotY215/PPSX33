# PS3 PPU/RSX Sample

`pong/pong.c` is a PlayStation 3-targeted C sample, not a native desktop game. It uses PSL1GHT headers and RSX/GCM display-buffer APIs, draws a Pong scene into a framebuffer, reads PS3 controller input, and includes a VMX/AltiVec operation probe.

The intended test path is:

1. Compile `pong/pong.c` with a compatible PSL1GHT `ppu-gcc` toolchain.
2. Produce `build/pong/pong.elf`.
3. Run the ELF through `ps3_cli` to create a project, lift PPU instructions, and invoke the host build.
4. Inspect the lift report and generated output.

The helper scripts are `scripts/build_pong_recompile.bat` and `scripts/build_pong_recompile.sh`. They require the PS3 homebrew SDK and a previously built PPSX33 CLI. This path is experimental and has not been marked as verified unless it has actually been run.

A successful host build does not mean the PS3 RSX/GCM calls or gameplay will work in PPSX33's current runtime. The sample folder intentionally contains only target-specific source code. Do not add copyrighted commercial-game assets or binaries.
