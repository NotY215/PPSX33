# Contributing to PPSX33

Thank you for contributing to PPSX33, an experimental PS3 ELF static-recompilation project. Contributions to the ELF loader, PPU lifter, generated-code runtime, CLI, WinForms GUI, tests, and documentation are welcome.

## Development environment

For Windows development, use Visual Studio 2022 or a newer compatible version with the **Desktop development with C++** workload, a Windows SDK, CMake, Ninja, and the .NET 8 SDK. The GUI targets `net8.0-windows`. See [Compiler Setup](Compilers-files/README.md) for the build and local tool-staging notes.

## Code and documentation standards

- Keep generated code deterministic and readable.
- Do not hand-edit generated files inside a game's `codebase/` directory.
- Keep `src/core/ps3core.h` synchronized with all consumers, including C# P/Invoke declarations.
- When PPU instruction translation changes, update [PPU coverage](docs/PPU_COVERAGE.md) and add focused regression tests.
- When a game lift changes, update the relevant record under `docs/games/` and the game coverage index.
- Keep static translation counts separate from instruction semantic correctness, runtime execution, and game compatibility.
- Clearly identify approximate instruction handling, no-op fallbacks, stubs, and heuristics.
- Do not claim that a build, test, or game was validated unless the relevant command was actually run and its result recorded.
- Do not commit game dumps, decrypted ELFs, EBOOT files, encryption keys, copyrighted game assets, generated commercial-game executables, or proprietary compiler binaries.
- Keep public setup instructions suitable for contributors who have not seen internal development conversations.

## Testing

On Windows, run `scripts\build_all.bat`, then `scripts\run_smoke_test.bat`. On Linux/macOS or a compatible MSYS environment, run `bash scripts/run_smoke_test.sh` with Python 3 and a C++17 compiler available. The smoke test uses a synthetic PPC64 ELF and expects the generated program to print `OK`; it is not a commercial-game compatibility test.

The optional PS3 Pong pipeline requires a compatible PSL1GHT toolchain and a built PPSX33 CLI. Do not mark that pipeline as verified unless it has been executed successfully.

## Pull requests

Keep each change focused. Describe the problem, implementation, test commands, results, and remaining limitations. Include regression tests for instruction semantics and runtime behavior where practical.

By submitting a contribution, you agree that it is provided under the project's Apache License 2.0, unless a different arrangement is agreed in writing.
