# PPSX33 Releases

Official PPSX33 project page: [https://notyvos.gt.tc/PPSX3/](https://notyvos.gt.tc/PPSX3/)

This directory contains release documentation for PPSX33. Check the project page for the current project information and available release downloads. A documented release is not necessarily a published downloadable build.

## PPSX33 Alpha 01

[Read the Alpha 01 release notes](PPSX33-Alpha-01.md).

Alpha 01 is a historical early preview. Its capabilities are limited to the exact binaries in that archive and may lag behind `main`. The current source tree includes a host build driver and GUI Build/Run workflow, but successful compilation does not establish correct runtime behavior or commercial-game compatibility. Check the specific archive's release notes and validate that exact package before assuming a feature is included.

### Current scope

- Load and analyze supported decrypted ELF64 big-endian PowerPC64 inputs.
- Run static PPU lifting and inspect its reports.
- Use the GUI/CLI features actually present in the downloaded package.
- For current source builds, exercise the host build workflow and synthetic smoke test.

### Not yet complete

- Alpha 01's exact build capabilities must be determined from that archive; documentation of newer source features does not update an already-published binary.
- Successful analysis, decompilation, or compilation does not guarantee that generated code can run correctly.
- Full PS3 system-library, SPU, RSX, and commercial-game compatibility is not established.

Use the release notes for package contents, limitations, and validation guidance. Please report problems with reproducible steps and logs, without uploading copyrighted game files or other restricted game data.

## Release status

Only treat a release as available when its download is published on the project page or an official release entry. Documentation alone does not mean that a binary package has been built, tested, or published.
