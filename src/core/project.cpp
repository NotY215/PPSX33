#include "project.h"
#include "elf_loader.h"
#include "ppu_lifter.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <atomic>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;
namespace ps3 {

#include "project_impl.inl"

} // namespace ps3
