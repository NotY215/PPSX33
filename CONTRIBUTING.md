# Contributing to PPSX33

<p align="center"><img src="https://raw.githubusercontent.com/NotY215/PPSX33/main/assets/ppsx33-logo.svg" alt="PPSX33 circular logo" width="120" /></p>


PPSX33 accepts focused contributions to the ELF loader, PPU lifter, generated-code runtime, CLI, WinForms GUI, regression tests, build scripts, and documentation.

## Development environment

Windows development uses Visual Studio 2022 or a compatible newer version with the **Desktop development with C++** workload, a Windows SDK, CMake, Ninja, and the .NET 8 SDK. The GUI targets `net8.0-windows`. See [Compiler Setup](Compilers-files/README.md) and the [Developer Guide](docs/DEVELOPER_GUIDE.md).

## Local setup and validation

1. Clone the repository and create a feature branch.
2. Configure a supported compiler environment.
3. Run `scripts\build_all.bat` on Windows.
4. Run `scripts\run_smoke_test.bat` on Windows, or `bash scripts/run_smoke_test.sh` on a compatible POSIX/MSYS environment.
5. Record the exact commands and results. If a command cannot be run, state that explicitly in the change description.

The smoke test uses a synthetic PPC64 ELF and expects the generated program to print `OK`. It does not test commercial-game compatibility. The optional Pong pipeline requires PSL1GHT and a built PPSX33 CLI.

## Engineering requirements

- Keep changes focused and preserve established interfaces unless an interface change is required.
- Keep `src/core/ps3core.h` synchronized with C/C++ consumers and C# P/Invoke declarations.
- Keep generated output deterministic where possible. Do not hand-edit generated files in a game's `codebase/` directory.
- For PPU opcode changes, verify operand decoding and architectural side effects, add focused regression tests, and update [PPU coverage](docs/PPU_COVERAGE.md).
- For a game report update, preserve exact tool output and update the matching record and [game index](docs/games/README.md).
- Distinguish translation counts, semantic correctness, runtime behavior, and game compatibility.
- Identify approximate behavior, stubs, heuristics, and fallbacks accurately.
- Do not claim a build, test, or game was validated unless the relevant command was executed and its result recorded.
- Do not commit decrypted ELFs, EBOOT files, game dumps, encryption keys, copyrighted assets, generated commercial-game executables, or proprietary compiler binaries.
- Use clear technical language and keep documentation suitable for external contributors.

## Pull request contents

Include the problem being solved, design and implementation details, affected components, tests run with results, and remaining limitations. Include regression tests for instruction semantics and runtime behavior where practical. Keep unrelated refactors out of the same change.

Contributions are distributed under the project's Apache License 2.0 unless a different arrangement is agreed in writing.
