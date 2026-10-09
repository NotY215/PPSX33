#!/usr/bin/env bash
# End-to-end smoke test on Linux/macOS/MSYS: ELF -> lift -> build -> run. Expects "OK".
set -e
cd "$(dirname "$0")/.."
python3 tests/make_test_elf.py /tmp/ps3_test.elf
g++ -std=c++17 src/cli/ps3_cli.cpp src/core/ps3core_api.cpp src/core/project.cpp src/core/elf_loader.cpp \
    src/core/ppu_lifter.cpp src/core/ps3_util.c -o /tmp/ps3_cli
rm -rf /tmp/ps3_root
/tmp/ps3_cli /tmp/ps3_root SmokeTest /tmp/ps3_test.elf runtime ""
cd /tmp/ps3_root/SmokeTest/output
./game 2>/dev/null | grep -qx OK && echo "SMOKE TEST PASSED" || { echo "SMOKE TEST FAILED"; exit 1; }
