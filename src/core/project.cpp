#include "project.h"
#include "elf_loader.h"
#include "ppu_lifter.h"
#include "ps3_util.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace ps3 {

std::string sanitize_game_name(const std::string& n) {
    std::string o; o.reserve(n.size());
    for (char c : n) {
        if (std::isalnum((unsigned char)c) || c == '_' || c == '-' || c == ' ') o.push_back(c);
    }
    while (!o.empty() && o.back() == ' ') o.pop_back();
    return o;
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
    if (ec) { err = ec.message(); return false; }
    project_dir = p.string();
    err.clear();
    return true;
}

bool lift_project(const std::string& project_dir, std::string& log, std::string& err) {
    log.clear();
    fs::path in = fs::path(project_dir) / "input" / "EBOOT.elf";
    if (!fs::exists(in)) { err = "Missing input/EBOOT.elf"; return false; }
    fs::path code = fs::path(project_dir) / "codebase";
    fs::create_directories(code);
    ElfImage img;
    if (!load_elf(in.string(), img, err)) return false;
    LiftResult lr;
    if (!lift_ppu(img, code.string(), lr, err)) return false;
    {
        std::ofstream ar(code / "analysis_report.txt");
        ar << "ELF segments: " << img.segments.size() << "\n";
        ar << "entry OPD: 0x" << std::hex << img.entry_opd << std::dec << "\n";
        ar << "symbols=" << img.symbols.size()
           << " opds=" << img.opds.size()
           << " prx_hits=" << img.prx_imports.size()
           << " spu_images=" << img.spu_images.size() << "\n";
        for (size_t i = 0; i < img.prx_imports.size() && i < 64; ++i) {
            ar << "  " << img.prx_imports[i].name
               << " nid=0x" << std::hex << img.prx_imports[i].nid << std::dec << "\n";
        }
        for (size_t i = 0; i < img.spu_images.size(); ++i) {
            char name[64];
            std::snprintf(name, sizeof name, "spu_image_%02zu.bin", i);
            std::ofstream sf(code / name, std::ios::binary);
            auto& blob = img.spu_images[i];
            sf.write(reinterpret_cast<const char*>(blob.data()), (std::streamsize)blob.size());
        }
    }
    {
        std::ofstream lrpt(code / "lift_report.txt");
        lrpt << "Instructions: " << lr.instructions
             << "  translated: " << lr.translated
             << "  unimplemented: " << lr.unimplemented
             << "  chunks: " << lr.chunks << "\n";
    }
    log = "ELF loaded: " + std::to_string(img.segments.size()) + " segments, entry OPD 0x" +
          ([&]{ char b[32]; std::snprintf(b,sizeof b,"%x", (unsigned)img.entry_opd); return std::string(b); })() +
          "\nAnalysis: symbols=" + std::to_string(img.symbols.size()) +
          " opds=" + std::to_string(img.opds.size()) +
          " prx_hits=" + std::to_string(img.prx_imports.size()) +
          " spu_images=" + std::to_string(img.spu_images.size()) +
          "\nInstructions: " + std::to_string(lr.instructions) +
          "  translated: " + std::to_string(lr.translated) +
          "  unimplemented: " + std::to_string(lr.unimplemented) +
          "  chunks: " + std::to_string(lr.chunks) +
          "\nSee codebase/lift_report.txt and codebase/analysis_report.txt.\n";
    err.clear();
    return true;
}

namespace {

std::string q(const std::string& s) { return "\"" + s + "\""; }

const char* kExe = ".exe"; const char* kDll = ".dll";

void live_log(const char* msg) {
    // best-effort: write to build_live.log if cwd is output
    std::ofstream f("build_live.log", std::ios::app);
    if (f) { f << msg; f.flush(); }
}

std::string find_vcvars64() {
    const char* roots[] = {
        "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        nullptr
    };
    for (int i = 0; roots[i]; ++i)
        if (fs::exists(roots[i])) return roots[i];
    return {};
}

std::string find_bundled_cl(const std::string& compilers_dir) {
    fs::path p = fs::path(compilers_dir) / "MSCV" / "cl.exe";
    if (fs::exists(p)) return p.string();
    return {};
}

void cleanup_after_build(const fs::path& project_dir, const fs::path& out, std::string& log) {
    std::error_code ec;
    fs::path codebase = project_dir / "codebase";
    fs::path input = project_dir / "input";
    for (const char* name : { "lift_report.txt", "analysis_report.txt" }) {
        fs::path src = codebase / name;
        if (fs::exists(src))
            fs::copy_file(src, out / name, fs::copy_options::overwrite_existing, ec);
    }
    if (fs::exists(out / "obj")) {
        fs::remove_all(out / "obj", ec);
        log += "cleanup: removed output/obj\n";
        live_log("cleanup: removed output/obj\n");
    }
    if (fs::exists(out / "src")) {
        fs::remove_all(out / "src", ec);
        log += "cleanup: removed output/src\n";
        live_log("cleanup: removed output/src\n");
    }
    for (const char* name : { "build.ninja", "build_msvc.bat", "link_rt.rsp", "link_game.rsp" }) {
        if (fs::exists(out / name)) fs::remove(out / name, ec);
    }
    if (fs::exists(codebase)) {
        for (auto& e : fs::directory_iterator(codebase, ec)) {
            std::string n = e.path().filename().string();
            if (n == "lift_report.txt" || n == "analysis_report.txt") continue;
            fs::remove_all(e.path(), ec);
        }
    }
    if (fs::exists(input)) {
        for (auto& e : fs::directory_iterator(input, ec)) {
            std::string n = e.path().filename().string();
            if (n == "EBOOT.elf") continue;
            fs::remove_all(e.path(), ec);
        }
    }
    log += "cleanup: done (exe/dll/guest_image.bin kept)\n";
    live_log("cleanup: done\n");
}

} // namespace

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log) {
    log.clear();
    fs::path out = fs::path(project_dir) / "output";
    fs::path src = out / "src";
    fs::path code = fs::path(project_dir) / "codebase";
    std::error_code ec;
    fs::create_directories(out, ec);
    fs::create_directories(src, ec);

    // copy runtime + generated sources into output/src
    if (fs::exists(runtime_dir)) {
        for (auto& e : fs::recursive_directory_iterator(runtime_dir, ec)) {
            if (!e.is_regular_file()) continue;
            fs::path rel = fs::relative(e.path(), runtime_dir, ec);
            fs::path dst = src / rel;
            fs::create_directories(dst.parent_path(), ec);
            fs::copy_file(e.path(), dst, fs::copy_options::overwrite_existing, ec);
        }
    }
    if (fs::exists(code)) {
        for (auto& e : fs::directory_iterator(code, ec)) {
            if (!e.is_regular_file()) continue;
            std::string n = e.path().filename().string();
            if (n.size() > 4 && (n.substr(n.size()-4) == ".cpp" || n.substr(n.size()-2) == ".h" ||
                                 n.substr(n.size()-4) == ".hpp"))
                fs::copy_file(e.path(), src / n, fs::copy_options::overwrite_existing, ec);
        }
    }

    live_log("PPSX33 build started\n");

    std::vector<std::string> rtSrc, gameSrc;
    for (auto& e : fs::directory_iterator(src, ec)) {
        if (!e.is_regular_file()) continue;
        std::string n = e.path().filename().string();
        if (n.size() < 5 || n.substr(n.size()-4) != ".cpp") continue;
        if (n.rfind("ps3rt", 0) == 0 || n == "rsx_stub.cpp" || n == "spu_stub.cpp" ||
            n == "audio_save.cpp" || n == "ps3rt_hw.cpp" || n == "ps3rt_log.cpp")
            rtSrc.push_back(n);
        else
            gameSrc.push_back(n);
    }
    std::sort(rtSrc.begin(), rtSrc.end());
    std::sort(gameSrc.begin(), gameSrc.end());

    int build_total = (int)rtSrc.size() + (int)gameSrc.size() + 2; // +2 link steps
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "BUILD_TOTAL=%d\n", build_total);
        live_log(buf);
        log += buf;
    }

    // Always brand the host binary as PPSX33.exe (tool identity, not a retail game title).
    std::string out_exe = std::string("PPSX33") + kExe;

#ifdef _WIN32
    std::string vcvars = find_vcvars64();
    std::string bundled_cl = find_bundled_cl(compilers_dir);
    bool use_msvc = !vcvars.empty() || !bundled_cl.empty();

    if (use_msvc) {
        {
            std::ofstream rt_rsp(out / "link_rt.rsp");
            rt_rsp << "/nologo\n/DLL\n/OUT:ps3rt.dll\n";
            for (auto& n : rtSrc) rt_rsp << "obj\\rt_" << n << ".obj\n";
            rt_rsp << "d3d11.lib\nd3d10.lib\ndxgi.lib\nuser32.lib\ngdi32.lib\nwinmm.lib\n";
        }
        {
            std::ofstream g_rsp(out / "link_game.rsp");
            g_rsp << "/nologo\n/OUT:\"" << out_exe << "\"\n";
            for (auto& n : gameSrc) g_rsp << "obj\\g_" << n << ".obj\n";
            if (fs::exists(src / "game_main.cpp") &&
                std::find(gameSrc.begin(), gameSrc.end(), "game_main.cpp") == gameSrc.end()) {
                g_rsp << "obj\\g_game_main.obj\n";
            }
            g_rsp << "ps3rt.lib\n";
        }

        fs::path bat = out / "build_msvc.bat";
        {
            std::ofstream f(bat);
            f << "@echo on\nsetlocal EnableExtensions\n";
            f << "echo === PPSX33 MSVC build ===\n";
            f << "echo BUILD_TOTAL=" << build_total << "\n";
            f << "echo cwd=%CD%\n";
            f << "set \"PATH=C:\\Windows\\system32;C:\\Windows;C:\\Windows\\System32\\Wbem\"\n";
            if (!vcvars.empty()) {
                f << "echo vcvars=" << vcvars << "\n";
                f << "call " << q(vcvars) << "\n";
                f << "if errorlevel 1 (echo vcvars FAILED & exit /b 1)\n";
            } else {
                f << "echo WARNING: vcvars64.bat not found — using PATH/bundled cl\n";
            }
            std::string cl = bundled_cl.empty() ? "cl" : q(bundled_cl);
            f << "echo cl=" << cl << "\n";
            f << "where cl 2>nul\n";
            f << "if not exist src\\ps3rt.cpp (echo MISSING src\\ps3rt.cpp & exit /b 1)\n";
            f << "if not exist obj mkdir obj\n";
            std::string cflags = "/nologo /O2 /EHsc /std:c++17 /bigobj /FS /I src /DPS3RT_BUILD_DLL=1";
            for (auto& n : rtSrc) {
                std::string o = "obj\\rt_" + n + ".obj";
                f << "echo COMPILE " << n << "\n";
                f << cl << " " << cflags << " /c src\\" << n << " /Fo" << o << " || exit /b 1\n";
            }
            f << "echo LINK ps3rt.dll\n";
            f << "link @link_rt.rsp || exit /b 1\n";
            for (auto& n : gameSrc) {
                std::string o = "obj\\g_" + n + ".obj";
                f << "echo COMPILE " << n << "\n";
                f << cl << " /nologo /O2 /EHsc /std:c++17 /bigobj /FS /I src /c src\\" << n << " /Fo" << o << " || exit /b 1\n";
            }
            if (fs::exists(src / "game_main.cpp") &&
                std::find(gameSrc.begin(), gameSrc.end(), "game_main.cpp") == gameSrc.end()) {
                f << "echo COMPILE game_main.cpp\n";
                f << cl << " /nologo /O2 /EHsc /std:c++17 /bigobj /FS /I src /c src\\game_main.cpp /Foobj\\g_game_main.obj || exit /b 1\n";
            }
            f << "echo LINK " << out_exe << "\n";
            f << "link @link_game.rsp || exit /b 1\n";
            f << "echo BUILD_OK\n";
            f << "exit /b 0\n";
        }

        // run bat from output dir
        std::string cmd = "cmd /C \"cd /D " + out.string() + " && build_msvc.bat\"";
        live_log(("> build_msvc.bat\n"));
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            log += "MSVC build failed (exit=" + std::to_string(rc) + ")\n";
            live_log(log.c_str());
            return false;
        }
        log += "MSVC build OK → " + out_exe + "\n";
        live_log("MSVC build OK\n");
        cleanup_after_build(fs::path(project_dir), out, log);
        return true;
    }
#endif
    log += "No MSVC toolchain found.\n";
    return false;
}

} // namespace ps3
