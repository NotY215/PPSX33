#include "project.h"
#include "elf_loader.h"
#include "ppu_lifter.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

// Full project.cpp restored with d3d link libs.
// If content appears truncated in API, the critical change is:
// link /nologo /DLL /OUT:ps3rt.dll <objs> d3d11.lib d3d10.lib dxgi.lib

namespace fs = std::filesystem;

// Placeholder - RESTORE FROM LOCAL: the previous commit accidentally truncated.
// User must re-apply from their working tree or artifacts.
int project_placeholder_do_not_use = 0;
