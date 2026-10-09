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
    ElfImage img;
    fs::path in = fs::path(project_dir) / "input" / "EBOOT.elf";
    if (!load_elf(in.string(), img, err)) return false;
    LiftStats st;
    if (!lift_elf(img, (fs::path(project_dir) / "codebase").string(), st, err)) return false;
    std::ostringstream o;
    o << "ELF loaded: " << img.segments.size() << " segments, entry OPD 0x" << std::hex << img.entry << std::dec << "\n"
      << "Instructions: " << st.instructions << "  translated: " << st.implemented
      << "  unimplemented: " << st.unimplemented << "  chunks: " << st.chunks << "\n"
      << "See codebase/lift_report.txt for the list of missing opcodes.\n";
    log = o.str();
    return true;
}

// ---------------- build driver ----------------
namespace {
#ifdef _WIN32
const char* kExe = ".exe"; const char* kDll = ".dll";
#else
const char* kExe = "";     const char* kDll = ".so";
#endif

struct Step { std::vector<std::string> in; std::string out; std::string cmd; };

std::string q(const std::string& s) { return "\"" + s + "\""; }

int run(const std::string& cmd, std::string& log) {
    log += "> " + cmd + "\n";
#ifdef _WIN32
    FILE* p = _popen(("\"" + cmd + " 2>&1\"").c_str(), "r");
#else
    FILE* p = popen((cmd + " 2>&1").c_str(), "r");
#endif
    if (!p) return -1;
    char buf[512];
    while (std::fgets(buf, sizeof buf, p)) log += buf;
#ifdef _WIN32
    return _pclose(p);
#else
    return pclose(p);
#endif
}

// Locate vcvars64.bat from a Visual Studio install (Community / Professional / BuildTools).
std::string find_vcvars64() {
#ifdef _WIN32
    // 1) vswhere (most reliable)
    const char* vswhere = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
    if (fs::exists(vswhere)) {
        FILE* p = _popen(
            "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe\" "
            "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
            "-property installationPath 2>nul", "r");
        if (p) {
            char buf[1024] = {};
            if (std::fgets(buf, sizeof buf, p)) {
                std::string path = buf;
                while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' '))
                    path.pop_back();
                fs::path vcvars = fs::path(path) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
                if (fs::exists(vcvars)) {
                    _pclose(p);
                    return vcvars.string();
                }
            }
            _pclose(p);
        }
    }

    // 2) Common install paths (VS 2022 / 2026 / BuildTools)
    const char* candidates[] = {
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\18\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat",
    };
    for (auto c : candidates)
        if (fs::exists(c)) return c;
#endif
    return {};
}

std::string find_tool(const fs::path& dir, const std::vector<std::string>& names) {
    for (const auto& n : names) {
        fs::path p = dir / (n + (std::string(kExe) == ".exe" ? ".exe" : ""));
        if (!fs::exists(p)) continue;
        if (n == "g++" || n == "c++" || n == "gcc") {
            if (!fs::exists(dir / "libexec")) continue; // incomplete MinGW
        }
        return p.string();
    }
    return names.front(); // PATH fallback
}

} // namespace

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err) {
    fs::path out = fs::path(project_dir) / "output";
    fs::path code = fs::path(project_dir) / "codebase";
    fs::path src = out / "src";
    std::error_code ec;
    fs::create_directories(src, ec);
    if (!fs::exists(code / "ppu_chunks.cpp")) {
        err = "Run the lift step first (codebase is empty).";
        return false;
    }

    const fs::copy_options ow = fs::copy_options::overwrite_existing;
    std::vector<std::string> gameSrc, rtSrc;
    for (auto& e : fs::directory_iterator(code)) {
        std::string n = e.path().filename().string();
        if (n.rfind("ppu_chunk", 0) == 0 || n == "game_main.cpp") {
            fs::copy_file(e.path(), src / n, ow, ec);
            if (e.path().extension() == ".cpp") gameSrc.push_back(n);
        } else if (n == "image_info.h") {
            fs::copy_file(e.path(), src / n, ow, ec);
        }
    }
    fs::copy_file(code / "guest_image.bin", out / "guest_image.bin", ow, ec);
    for (const char* n : {"ppu_runtime.h", "ps3rt.cpp", "spu_stub.cpp", "rsx_stub.cpp"}) {
        fs::path from = fs::path(runtime_dir) / n;
        if (!fs::exists(from)) { err = "Runtime file missing: " + from.string(); return false; }
        fs::copy_file(from, src / n, ow, ec);
        if (std::string(n).find(".cpp") != std::string::npos) rtSrc.push_back(n);
    }

    fs::create_directories(out / "obj", ec);

#ifdef _WIN32
    // ---------- Prefer MSVC (Visual Studio Community) ----------
    std::string vcvars = find_vcvars64();
    bool use_msvc = !vcvars.empty();

    if (use_msvc) {
        log += "Using MSVC via: " + vcvars + "\n";

        // Write a single build.bat that sets up the VS environment once, then compiles everything.
        // This avoids the need for MinGW entirely when VS is installed.
        fs::path bat = out / "build_msvc.bat";
        {
            std::ofstream f(bat);
            f << "@echo off\n"
                 "setlocal\n"
                 "call " << q(vcvars) << " || exit /b 1\n"
                 "cd /d " << q(out.string()) << "\n"
                 "if not exist obj mkdir obj\n"
                 "\n";

            // Runtime -> ps3rt.dll
            std::string rt_objs;
            for (auto& n : rtSrc) {
                std::string obj = "obj\\rt_" + n + ".obj";
                // /O2 /std:c++17 /EHsc /MD /DPS3RT_BUILD_DLL /I src
                f << "cl /nologo /O2 /std:c++17 /EHsc /MD /DPS3RT_BUILD_DLL=1 /I src "
                     "/c src\\" << n << " /Fo" << obj << " || exit /b 1\n";
                rt_objs += " " + obj;
            }
            f << "link /nologo /DLL /OUT:ps3rt.dll" << rt_objs << " || exit /b 1\n\n";

            // Generated game code -> game.exe
            std::string game_objs;
            for (auto& n : gameSrc) {
                std::string obj = "obj\\g_" + n + ".obj";
                f << "cl /nologo /O2 /std:c++17 /EHsc /MD /I src "
                     "/c src\\" << n << " /Fo" << obj << " || exit /b 1\n";
                game_objs += " " + obj;
            }
            f << "link /nologo /OUT:game.exe" << game_objs << " ps3rt.lib || exit /b 1\n"
                 "echo MSVC build OK\n"
                 "exit /b 0\n";
        }

        auto prev = fs::current_path(ec);
        fs::current_path(out, ec);
        int rc = run("cmd /c build_msvc.bat", log);
        fs::current_path(prev, ec);

        if (rc != 0) {
            err = "MSVC build failed. See log.";
            return false;
        }
        log += "Build OK: output/game.exe + output/ps3rt.dll + output/guest_image.bin (MSVC)\n";
        return true;
    }
#endif

    // ---------- Fallback: g++ / MinGW ----------
    fs::path cdir(compilers_dir);
    std::string cxx = find_tool(cdir, {"g++", "c++"});
    std::string ninja = find_tool(cdir, {"ninja"});

    log += "MSVC (vcvars64.bat) not found. Falling back to g++.\n";
    if (cxx == "g++" || cxx == "c++") {
        log += "Using system g++ from PATH.\n";
    }

    std::vector<Step> steps;
    std::string cflags = "-O2 -std=c++17 -I src";
    std::vector<std::string> gameObjs, rtObjs;

    for (auto& n : rtSrc) {
        std::string o = "obj/rt_" + n + ".o";
        steps.push_back({{ "src/" + n }, o,
            q(cxx) + " " + cflags + " -DPS3RT_BUILD_DLL=1 -fPIC -c src/" + n + " -o " + o});
        rtObjs.push_back(o);
    }
    std::string dll = std::string("ps3rt") + kDll;
    {
        std::string objs;
        for (auto& o : rtObjs) objs += " " + o;
        steps.push_back({rtObjs, dll, q(cxx) + " -shared" + objs + " -o " + dll});
    }
    for (auto& n : gameSrc) {
        std::string o = "obj/g_" + n + ".o";
        steps.push_back({{ "src/" + n }, o,
            q(cxx) + " " + cflags + " -c src/" + n + " -o " + o});
        gameObjs.push_back(o);
    }
    {
        std::string objs;
        for (auto& o : gameObjs) objs += " " + o;
        std::string rpath;
#ifndef _WIN32
        rpath = " -Wl,-rpath,'$ORIGIN'";
#endif
        std::string exe = std::string("game") + kExe;
        std::vector<std::string> ins = gameObjs;
        ins.push_back(dll);
        steps.push_back({ins, exe, q(cxx) + objs + " ./" + dll + rpath + " -o " + exe});
    }

    {
        std::ofstream n(out / "build.ninja");
        n << "# GENERATED by ps3core build driver (g++ fallback)\n"
             "rule run\n  command = $cmd\n  description = $out\n\n";
        for (auto& s : steps) {
            std::string c = s.cmd, esc;
            for (char ch : c) { if (ch == '$') esc += "$$"; else esc += ch; }
            n << "build " << s.out << ": run";
            for (auto& i : s.in) n << " " << i;
            n << "\n  cmd = " << esc << "\n";
        }
    }

    auto prev = fs::current_path(ec);
    fs::current_path(out, ec);
    int rc = 0;
    if (fs::exists(ninja)) {
        rc = run(q(ninja), log);
    } else {
        log += "(ninja not found, compiling sequentially)\n";
        for (auto& s : steps) {
            rc = run(s.cmd, log);
            if (rc != 0) break;
        }
    }
    fs::current_path(prev, ec);

    if (rc != 0) {
        err = "Build failed. See log.";
        return false;
    }
    log += "Build OK: output/game" + std::string(kExe) + " + output/" + dll + " + output/guest_image.bin (g++)\n";
    return true;
}

} // namespace ps3
