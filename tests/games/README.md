# PS3 PPU/RSX Sample

`pong/pong.c` is a PlayStation 3-targeted C sample, not a desktop game. It uses PSL1GHT headers and RSX/GCM display-buffer APIs, draws into a framebuffer, reads PS3 controller input, and includes a VMX/AltiVec operation probe.

## Test workflow

1. Compile `pong/pong.c` with a compatible PSL1GHT `ppu-gcc` toolchain.
2. Produce `build/pong/pong.elf`.
3. Run the ELF through `ps3_cli` to create a project, lift PPU instructions, and invoke the host build.
4. Inspect lift and analysis reports, then review generated artifacts.
5. Record the exact commands and results before marking the pipeline verified.

Helper scripts:

- `scripts/build_pong_recompile.bat`
- `scripts/build_pong_recompile.sh`

The workflow requires the PS3 homebrew SDK and a built PPSX33 CLI. A successful host build does not prove RSX/GCM calls or gameplay work in the current runtime. Keep this directory limited to target-specific sample source; do not add copyrighted commercial-game assets or binaries.
