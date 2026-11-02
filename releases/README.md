# PPSX33 Releases

Official PPSX33 project page: [https://ppsx33.xo.je/](https://ppsx33.xo.je/)

This directory contains release documentation for PPSX33. Check the project page or an official release entry for published downloads. A release note is not, by itself, evidence that a downloadable build has been packaged, tested, or published.

## PPSX33 Alpha 1.0.4

[Read the Alpha 1.0.4 release notes](PPSX33-Alpha-1.0.4.md).

Alpha 1.0.4 documents the current development direction compared with the historical Alpha 01 package, including the Qt 6 desktop application, modular `build/dist/` output layout, Vulkan-first host presentation, and clearer build and validation expectations. The notes describe the current repository direction and must not be treated as proof that every capability is present in a particular archive.

## PPSX33 Alpha 01

[Read the Alpha 01 release notes](PPSX33-Alpha-01.md).

Alpha 01 is a historical early preview. Its capabilities are limited to the exact binaries in that archive and may lag behind `main`. The current source tree and newer documentation do not update the old package retroactively.

### Current project scope

- Load and analyze supported decrypted ELF inputs.
- Lift supported PPU code into generated C++ and inspect analysis/lift reports.
- Use the GUI/CLI workflows present in the checked-out source or downloaded package.
- Build native Windows output through the documented host toolchain.
- Validate runtime behavior with logs and reproducible tests.

### Not yet complete

- PPU semantics and runtime behavior remain partial.
- LV2, PRX, NID, and cell-library coverage remains incomplete.
- SPU, MFC/DMA, SPURS, and RSX/GCM functionality remain in progress.
- A successful analysis, lift, or compile does not guarantee booting, correct rendering, or playability.
- Compatibility with commercial PS3 games is not generally established.

Use each version's release notes for its scope, package details, limitations, and validation guidance. Report issues with reproducible steps and logs, without uploading copyrighted game files, firmware, keys, or restricted game data.

## Release status

Only treat a release as available when its download is published on the official project page or an official release entry. Documentation alone does not mean a binary package has been built, tested, or published.
