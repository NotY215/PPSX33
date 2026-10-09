#!/usr/bin/env bash
# Build PS3 PPC64 Pong ELF, then lift and recompile it with PPSX33.
set -euo pipefail
cd "$(dirname "$0")/.."
: "${PSL1GHT:?Configure the PS3 homebrew SDK and set PSL1GHT}"
command -v ppu-gcc >/dev/null || { echo "ERROR: ppu-gcc not found in PATH" >&2; exit 2; }
test -x build/dist/ps3_cli || { echo "ERROR: build/dist/ps3_cli missing; run scripts/build_all.bat first" >&2; exit 2; }
mkdir -p build/pong
echo "[1/3] Compile PS3 PPU/VMX source to ELF..."
ppu-gcc -O2 -Wall -Wextra -maltivec -mabi=altivec \
  -I"$PSL1GHT/ppu/include" tests/games/pong/pong.c \
  -L"$PSL1GHT/ppu/lib" -lrsx -lgcm_sys -lsysutil -lio -lm -o build/pong/pong.elf
echo "[2/3] Lift and build using PPSX33..."
build/dist/ps3_cli build/pong/project Pong build/pong/pong.elf runtime Compilers-files
echo "[3/3] Check generated artifact and lift report..."
test -f build/pong/project/Pong/output/game || test -f build/pong/project/Pong/output/game.exe || {
  echo "ERROR: native executable not found; inspect lift_report.txt" >&2; exit 1;
}
find build/pong/project/Pong -name lift_report.txt -print
echo "Pipeline completed. Review the report for unsupported opcode families."
