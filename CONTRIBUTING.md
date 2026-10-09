# Contributing to PPSX33

Thank you for contributing to PPSX33.

## Development environment

Use the latest stable Visual Studio 2026 (version 18) with the **Desktop development with C++** workload and a Windows SDK for Windows builds. Install the .NET components required by the GUI project, which currently targets `net8.0-windows`. See [Compiler Setup](Compilers-files/README.md) for local MSVC tool setup.

## Code and documentation standards

- Keep generated code deterministic and readable.
- Do not hand-edit generated files inside a game's `codebase/` directory.
- Keep `src/core/ps3core.h` synchronized with all consumers, including C# P/Invoke declarations.
- When instruction support changes, update `docs/PPU_COVERAGE.md` and add or extend regression tests.
- Update the README, roadmap, and relevant game coverage records when verified project status changes.
- Separate compilation success from runtime correctness and game compatibility in documentation.
- Do not commit game dumps, decrypted ELFs, EBOOT files, encryption keys, copyrighted game assets, generated commercial-game executables, or proprietary compiler binaries.
- Keep implementation notes and contributor-only workflow details in this file rather than user-facing setup guides.

## Testing

Run `scripts/run_smoke_test.sh` where supported. On Windows, run `scripts\build_all.bat` and the relevant runtime tests. Describe exactly which checks passed and which remain unverified. Add regression tests for instruction handling and runtime behavior where practical.

## Pull requests

Keep each pull request focused. Describe the problem, implementation, test commands, results, and any remaining limitations. Do not describe a feature as tested unless the relevant test was actually run.

By submitting a contribution, you agree that it is provided under the project's Apache License 2.0, unless a different arrangement is agreed in writing.
