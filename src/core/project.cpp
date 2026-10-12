#include "project.h"
#include "elf_loader.h"
#include "ppu_lifter.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace ps3 {

// EMERGENCY RESTORE NOTE:
// Full project.cpp was corrupted by a bad push. Re-clone from tag/commit a5b2b6e
// or re-apply from local backup. This minimal stub keeps the tree buildable for headers only.
//
// The copy list MUST include ps3rt_log.cpp:
// for (const char* n : {"ppu_runtime.h", "nid_table.h", "ps3rt.cpp", "ps3rt_log.cpp", "spu_stub.cpp", "rsx_stub.cpp"})

std::string sanitize_game_name(const std::string& n) {
    std::string r;
    for (char ch : n) r += (std::string("<>:\"/\\|?*").find(ch) != std::string::npos || (unsigned char)ch < 32) ? '_' : ch;
    while (!r.empty() && (r.back() == ' ' || r.back() == '.')) r.pop_back();
    return r;
}

bool create_project(const std::string& root, const std::string& game, const std::string& elf,
                    std::string& project_dir, std::string& err) {
    std::string name = sanitize_game_name(game);
    if (name.empty()) { err = "Game name is empty."; return false; }
    if (!fs::exists(elf)) { err = "ELF file does not exist: " + elf; return false; }
    std::error_code ec;
    fs::path p = fs::path(root) / name;
    fs::create_directories(p / "input", ec);
    fs::create_directories(p / "codebase", ec);
    fs::create_directories(p / "output", ec);
    if (ec) { err = "Cannot create folders: " + ec.message(); return false; }
    fs::copy_file(elf, p / "input" / "EBOOT.elf", fs::copy_options::overwrite_existing, ec);
    if (ec) { err = "Cannot copy ELF: " + ec.message(); return false; }
    project_dir = p.string();
    return true;
}

bool lift_project(const std::string& project_dir, std::string& log, std::string& err) {
    err = "lift_project: restore full project.cpp from commit a5b2b6e902355af24a8be30c2dbbed4eaf75d36f";
    return false;
}

bool prepare_runtime_build(const std::string& project_dir, const std::string& runtime_dir, std::string& err) {
    err = "prepare_runtime_build: restore full project.cpp";
    return false;
}

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err) {
    err = "build_project: restore full project.cpp from commit a5b2b6e";
    return false;
}

} // namespace ps3
