# PPSX33 Release Redistributables

This document lists the software a user may need to launch and use a **prebuilt PPSX33 Windows release**. It is for release users, not developers building PPSX33 from source.

## Required for the patched/prebuilt release

- **Windows SDK: install only if the release workflow requires SDK components.** Download it from Microsoft's official [Windows SDK downloads](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads). The SDK is primarily a development kit, not a general runtime dependency for launching a desktop application. Most users should not need to install it just to open PPSX33; install it when a packaged build step or a missing SDK component specifically requires it.
- **Microsoft Visual C++ Redistributable (x64):** May be required by native PPSX33 binaries or generated programs if they dynamically link the MSVC runtime. Install the current supported x64 package from Microsoft's [Latest supported Visual C++ Redistributable downloads](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170). Whether it is required depends on the exact binaries shipped in the release. If the release starts and its native dependencies are satisfied, do not install it unnecessarily.

## Not required for the patched/prebuilt release

- **.NET SDK or .NET Desktop Runtime:** The patched release ships the built UI files, and the release is not intended to require users to build the UI. Do not list .NET as a prerequisite for this package. This assumes the distributed UI executable is published in a way that includes its required .NET runtime; verify this on a clean Windows machine before release. If the actual packaged executable is framework-dependent, that package must be republished appropriately rather than asking users to install .NET contrary to the release requirements.
- **MSVC / Visual Studio / Build Tools:** These are development tools, not normal end-user launch prerequisites. The release already includes the `MSCV/` directory as stated in the package layout, so users should not be told to install MSVC separately just to launch the patched release. A compiler may still be needed for workflows that compile generated host code; that is distinct from launching the prebuilt application.
- **CMake and Ninja:** Build tools used by the project workflow, not prerequisites for simply launching the GUI. They may be used for host compilation if included and configured in the package.

## Release validation checklist

Before publishing, test the exact release archive on a clean supported Windows x64 environment without Visual Studio or a preinstalled .NET runtime:

1. Launch `PS3Recomp.exe`.
2. Launch `ps3_cli.exe --help`.
3. Run the intended packaged build workflow and note whether the Windows SDK or a Visual C++ Redistributable is actually needed.
4. If a missing DLL or runtime error appears, identify the dependency from the actual binary and add only the matching prerequisite.
5. Confirm whether the UI was published self-contained. The presence of `PS3Recomp.deps.json` and `PS3Recomp.runtimeconfig.json` alone does not prove that the runtime is bundled.

Do not claim that the Windows SDK or Visual C++ Redistributable is mandatory for every installation until the exact release binaries have been checked. Native runtime requirements can differ between the GUI, CLI, core DLL, and generated game output.

## Official downloads

- [Windows SDK downloads](https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads)
- [Latest supported Visual C++ Redistributable downloads](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)

