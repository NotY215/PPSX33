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

// NOTE: Full build_project implementation is in project_build.inl
// This thin file restores after accidental truncation.

static std::atomic<bool> g_build_cancel_requested{false};
static FILE* g_live = nullptr;
static std::string g_build_out_dir;
static unsigned long g_build_pid = 0;

static void live_open(const fs::path& out) {
    if (g_live) { fclose(g_live); g_live = nullptr; }
    fs::path p = out / "build_live.log";
    g_live = fopen(p.string().c_str(), "w");
}
static void live_log(const std::string& s) {
    if (g_live) { fputs(s.c_str(), g_live); fflush(g_live); }
}
static void live_close() {
    if (g_live) { fclose(g_live); g_live = nullptr; }
}

std::string sanitize_game_name(const std::string& n) {
    std::string o; o.reserve(n.size());
    for (unsigned char c : n) {
        if (std::isalnum(c) || c == '_' || c == '-' || c == ' ') o.push_back((char)c);
    }
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o.empty() ? "project" : o;
}

bool create_project(const std::string& root, const std::string& game, const std::string& elf,
                    std::string& project_dir, std::string& err) {
    std::string name = sanitize_game_name(game);
    if (name.empty()) { err = "Game name is empty."; return false; }
    std::error_code ec;
    fs::path p = fs::path(root) / name;
    fs::create_directories(p / "input", ec);
    fs::create_directories(p / "codebase", ec);
    fs::create_directories(p / "output", ec);
    fs::copy_file(elf, p / "input" / "EBOOT.elf", fs::copy_options::overwrite_existing, ec);
    if (ec) { err = "Failed to copy ELF: " + ec.message(); return false; }
    project_dir = p.string();
    err.clear();
    return true;
}

bool lift_project(const std::string& project_dir, std::string& log, std::string& err);
bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err);
void begin_build();
void cancel_build();

#include "project_build.inl"

} // namespace ps3
