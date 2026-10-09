// ps3_cli - command line front end for the same C API the GUI uses.
// Handy for CI, debugging and for other AIs: no GUI needed.
//   ps3_cli <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>
#include "../core/ps3core.h"
#include <cstdio>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 6) {
        std::fprintf(stderr, "usage: %s <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>\n", argv[0]);
        return 2;
    }
    std::vector<char> dir(4096), err(4096), log(1 << 20);
    if (ps3_create_project(argv[1], argv[2], argv[3], dir.data(), (int)dir.size(), err.data(), (int)err.size())) {
        std::fprintf(stderr, "create: %s\n", err.data()); return 1;
    }
    std::printf("project: %s\n", dir.data());
    if (ps3_lift_project(dir.data(), log.data(), (int)log.size())) { std::fprintf(stderr, "lift: %s\n", log.data()); return 1; }
    std::printf("%s", log.data());
    int rc = ps3_build_project(dir.data(), argv[4], argv[5], log.data(), (int)log.size());
    std::printf("%s\n", log.data());
    return rc;
}
