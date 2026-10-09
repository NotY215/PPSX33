# Contributing

Thanks for helping improve PS3 Recompiler.

## Before opening a change

- Check the current [README](README.md), [roadmap](ROADMAP.md), and [PPU coverage](docs/PPU_COVERAGE.md).
- Keep generated code deterministic and readable.
- Do not hand-edit files generated inside a game's `codebase/` directory.
- Do not commit game dumps, decrypted ELFs, EBOOT files, encryption keys, copyrighted game assets, or toolchain binaries.
- Keep the native C API in `src/core/ps3core.h` synchronized with its consumers, including the C# P/Invoke declarations.
- Update the README status and roadmap when implementation status changes.

## Testing

Run the available smoke test with `scripts/run_smoke_test.sh` where supported. For Windows changes, run the relevant build scripts and document any test you could not run. Add regression coverage for new instruction handling or runtime behavior when practical.

## Pull requests

Keep each pull request focused. Describe the problem, implementation, testing performed, and any known limitations. Never claim a feature is tested unless it was actually tested.

By submitting a contribution, you agree that it is provided under the project's Apache License 2.0, unless a different arrangement is agreed in writing.
