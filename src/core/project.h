// project.h - Phase 1 project folder handling + Phase 6 build driver.
#pragma once
#include <string>

namespace ps3 {
std::string sanitize_game_name(const std::string& name);
bool create_project(const std::string& root, const std::string& game, const std::string& elf,
                    std::string& project_dir, std::string& err);
bool lift_project(const std::string& project_dir, std::string& log, std::string& err);
bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err);
}
