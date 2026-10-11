# PPSX33 Releases

Official PPSX33 project page: [https://notyvos.gt.tc/PPSX3/](https://notyvos.gt.tc/PPSX3/)

This directory contains release documentation for PPSX33. Check the project page for the current project information and available release downloads. A documented release is not necessarily a published downloadable build.

## PPSX33 Alpha 01

[Read the Alpha 01 release notes](PPSX33-Alpha-01.md).

Alpha 01 is an early, limited preview. Its capabilities are restricted to features implemented in the distributed build. In particular, the current workflow can analyze and decompile/lift supported input files, but building the generated files into a working executable is not fully implemented yet. Do not assume that the presence of a GUI, CLI, generated C/C++ files, runtime sources, or compiler tools means the full build-and-run pipeline works end to end.

### Current scope

- Load and analyze supported decrypted ELF64 big-endian PowerPC64 inputs.
- Run the implemented static PPU decompilation/lifting workflow and inspect its reports.
- Evaluate the GUI and CLI functionality that is actually present in the Alpha package.

### Not yet complete

- Building generated output into a working executable is not fully implemented.
- Successful analysis or decompilation does not guarantee that generated files can be compiled, linked, launched, or run correctly.
- Full PS3 system-library, SPU, RSX, and commercial-game compatibility is not established.

Use the release notes for package contents, limitations, and validation guidance. Please report problems with reproducible steps and logs, without uploading copyrighted game files or other restricted game data.

## Release status

Only treat a release as available when its download is published on the project page or an official release entry. Documentation alone does not mean that a binary package has been built, tested, or published.
