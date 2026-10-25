#!/usr/bin/env bash
# Compile PS3 Pong ELF, lift and recompile via PPSX33 (system toolchain).
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ -z "${PSL1GHT:-}" ]]; then
  echo "ERROR: set PSL1GHT to the PS3 homebrew SDK root"
  exit 2
fi
command -v ppu-gcc >/dev/null || { echo "ERROR: ppu-gcc not in PATH"; exit 2; }
[[ -x build/dist/ps3_cli ]] || { echo "ERROR: build/dist/ps3_cli missing — run cmake build first"; exit 2; }
mkdir -p build/pong
echo "[1/3] Compile Pong ELF..."
ppu-gcc -O2 -Wall -Wextra -maltivec -mabi=altivec \
  -I"$PSL1GHT/ppu/include" tests/games/pong/pong.c \
  -L"$PSL1GHT/ppu/lib" -lrsx -lgcm_sys -lsysutil -lio -lm \
  -o build/pong/pong.elf
echo "[2/3] Lift + host build (redist path, no bundled compilers)..."
build/dist/ps3_cli build/pong/project Pong build/pong/pong.elf runtime redist
echo "[3/3] Check output..."
if [[ -f build/pong/project/Pong/output/game.exe ]] || [[ -f build/pong/project/Pong/output/game ]]; then
  echo "OK: generated game binary present"
else
  echo "ERROR: native binary missing — see lift_report.txt"
  exit 1
fi
