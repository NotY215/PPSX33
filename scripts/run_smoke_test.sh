#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ -x build/dist/ps3_cli ]] || { echo "ERROR: build/dist/ps3_cli missing"; exit 1; }
echo "[1/4] Generating test ELF..."
python3 tests/make_test_elf.py build/test.elf
echo "[2/4] Lift + build..."
rm -rf build/smoke_root
build/dist/ps3_cli build/smoke_root SmokeTest build/test.elf runtime redist
echo "[3/4] Run..."
( cd build/smoke_root/SmokeTest/output && ./game > ../../../smoke_out.txt 2>&1 || ./game.exe > ../../../smoke_out.txt 2>&1 )
echo "[4/4] Check OK..."
grep -qx "OK" build/smoke_out.txt
echo "SMOKE TEST PASSED"
