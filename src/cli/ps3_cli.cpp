// ps3_cli - command line front end for the same C API the GUI uses.
// Handy for CI and debugging. Normal users should run PS3Recomp.exe (the GUI).
//
// Usage:
//   ps3_cli <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>
//
// Exit codes:
//   0 = success
//   1 = create / lift / build failed
//   2 = bad arguments (missing args or --help)

#include "../core/ps3core.h"
#include <cstdio>
#include <cstring>
#include <vector>

static void print_usage(const char* argv0) {
    std::fprintf(stderr,
        "PS3 Recompiler CLI  (ps3core API v%d)\n"
        "\n"
        "Usage:\n"
        "  %s <root_dir> <game_name> <elf_path> <runtime_dir> <compilers_dir>\n"
        "\n"
        "Arguments:\n"
        "  root_dir       Folder where <game_name>/ will be created\n"
        "  game_name      Project name (sanitized to a folder name)\n"
        "  elf_path       Path to a decrypted ELF64-BE PPC64 file\n"
        "  runtime_dir    Path to the runtime/ sources\n"
        "  compilers_dir  Path to Compilers-files/ (g++.exe / ninja.exe) or \"\"\n"
        "\n"
        "Normal users should run the GUI instead:  PS3Recomp.exe\n",
        ps3_core_version(), argv0);
}

int main(int argc, char** argv) {
    if (argc == 2 && (std::strcmp(argv[1], "-h") == 0 ||
                      std::strcmp(argv[1], "--help") == 0 ||
                      std::strcmp(argv[1], "/?") == 0)) {
        print_usage(argv[0]);
        return 2;
    }

    if (argc < 6) {
        print_usage(argv[0]);
        return 2;
    }

    std::vector<char> dir(4096), err(4096), log(1 << 20);

    if (ps3_create_project(argv[1], argv[2], argv[3],
                           dir.data(), (int)dir.size(),
                           err.data(), (int)err.size()) != 0) {
        std::fprintf(stderr, "create failed: %s\n", err.data());
        return 1;
    }
    std::printf("project: %s\n", dir.data());

    if (ps3_lift_project(dir.data(), log.data(), (int)log.size()) != 0) {
        std::fprintf(stderr, "lift failed: %s\n", log.data());
        return 1;
    }
    std::printf("%s", log.data());

    int rc = ps3_build_project(dir.data(), argv[4], argv[5],
                               log.data(), (int)log.size());
    std::printf("%s\n", log.data());
    if (rc != 0) {
        std::fprintf(stderr, "build failed (exit %d)\n", rc);
        return 1;
    }
    return 0;
}
