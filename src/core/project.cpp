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

static std::atomic<bool> g_build_cancel_requested{false};
static FILE* g_live = nullptr;

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

bool lift_project(const std::string& project_dir, std::string& log, std::string& err) {
    ElfImage img;
    fs::path in = fs::path(project_dir) / "input" / "EBOOT.elf";
    if (!load_elf(in.string(), img, err)) return false;
    analyze_elf(img);
    LiftStats st;
    fs::path code = fs::path(project_dir) / "codebase";
    fs::create_directories(code);
    if (!lift_elf(img, code.string(), st, err)) return false;

    {
        std::ofstream ar(code / "analysis_report.txt");
        ar << "entry_opd: 0x" << std::hex << img.entry << std::dec << "\n";
        ar << "segments: " << img.segments.size() << "\n";
        ar << "symbols: " << img.symbols.size() << "\n";
        ar << "opd_entries: " << img.opds.size() << "\n";
        ar << "prx_string_hits: " << img.prx_imports.size() << "\n";
        ar << "embedded_spu_images: " << img.spu_images.size() << "\n";
        ar << "\nOPD samples (up to 32):\n";
        for (size_t i = 0; i < img.opds.size() && i < 32; ++i) {
            ar << "  [" << i << "] desc=0x" << std::hex << img.opds[i].addr
               << " entry=0x" << img.opds[i].entry
               << " toc=0x" << img.opds[i].toc << std::dec << "\n";
        }
        ar << "\nPRX/module string samples (up to 64):\n";
        for (size_t i = 0; i < img.prx_imports.size() && i < 64; ++i) {
            ar << "  " << img.prx_imports[i].name
               << "  nid=0x" << std::hex << img.prx_imports[i].nid << std::dec
               << "  @0x" << std::hex << img.prx_imports[i].stub_addr << std::dec << "\n";
        }
        ar << "\nEmbedded SPU images:\n";
        for (size_t i = 0; i < img.spu_images.size(); ++i) {
            ar << "  [" << i << "] host_va=0x" << std::hex << img.spu_images[i].host_addr
               << std::dec << " size=" << img.spu_images[i].data.size() << "\n";
            char name[64];
            std::snprintf(name, sizeof name, "spu_image_%02zu.bin", i);
            std::ofstream sf(code / name, std::ios::binary);
            sf.write((const char*)img.spu_images[i].data.data(),
                     (std::streamsize)img.spu_images[i].data.size());
        }
        size_t funcs = 0;
        for (const auto& s : img.symbols) if (s.is_func()) ++funcs;
        ar << "\nsymbol_functions: " << funcs << "\n";
    }

    {
        std::ofstream lrpt(code / "lift_report.txt");
        lrpt << "Instructions: " << st.instructions
             << "  translated: " << st.implemented
             << "  unimplemented: " << st.unimplemented
             << "  chunks: " << st.chunks << "\n";
    }

    std::ostringstream o;
    o << "ELF loaded: " << img.segments.size() << " segments, entry OPD 0x" << std::hex << img.entry << std::dec << "\n"
      << "Analysis: symbols=" << img.symbols.size()
      << " opds=" << img.opds.size()
      << " prx_hits=" << img.prx_imports.size()
      << " spu_images=" << img.spu_images.size() << "\n"
      << "Instructions: " << st.instructions
      << "  translated: " << st.implemented
      << "  unimplemented: " << st.unimplemented
      << "  chunks: " << st.chunks << "\n"
      << "See codebase/lift_report.txt and codebase/analysis_report.txt.\n";
    log = o.str();
    err.clear();
    return true;
}

namespace {

std::string q(const std::string& s) { return "\"" + s + "\""; }
const char* kExe = ".exe";

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

int run_cmd(const std::string& cmd, std::string& log) {
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) { log += "failed to start: " + cmd + "\n"; return -1; }
    char buf[512];
    while (fgets(buf, sizeof buf, pipe)) {
        log += buf;
        live_log(buf);
        if (g_build_cancel_requested.load()) break;
    }
#ifdef _WIN32
    int rc = _pclose(pipe);
#else
    int rc = pclose(pipe);
#endif
    return rc;
}

void cleanup_after_build(const std::string& project_dir, const fs::path& out, std::string& log) {
    std::error_code ec;
    fs::path codebase = fs::path(project_dir) / "codebase";
    fs::path input = fs::path(project_dir) / "input";
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
            if (n == "lift_report.txt" || n == "analysis_report.txt" || n == "guest_image.bin") continue;
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
    log += "cleanup: done (exe/dll kept)\n";
    live_log("cleanup: done\n");
}

} // namespace

void begin_build() { g_build_cancel_requested.store(false); }
void cancel_build() { g_build_cancel_requested.store(true); }

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err) {
    log.clear();
    err.clear();
    fs::path out = fs::path(project_dir) / "output";
    fs::path src = out / "src";
    fs::path code = fs::path(project_dir) / "codebase";
    std::error_code ec;
    fs::create_directories(out, ec);
    fs::create_directories(src, ec);
    live_open(out);

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
            if (n == "guest_image.bin")
                fs::copy_file(e.path(), out / n, fs::copy_options::overwrite_existing, ec);
        }
    }

    live_log("PPSX33 build started\n");

    std::vector<std::string> rtSrc, gameSrc;
    for (auto& e : fs::directory_iterator(src, ec)) {
        if (!e.is_regular_file()) continue;
        std::string n = e.path().filename().string();
        if (n.size() < 5 || n.substr(n.size()-4) != ".cpp") continue;
        if (n == "ps3rt.cpp" || n == "rsx_stub.cpp" || n == "spu_stub.cpp" ||
            n == "audio_save.cpp" || n == "assets.cpp") {
            rtSrc.push_back(n);
        } else if (n == "ps3rt_hw.cpp" || n == "ps3rt_log.cpp") {
            continue;
        } else {
            gameSrc.push_back(n);
        }
    }
    std::sort(rtSrc.begin(), rtSrc.end());
    std::sort(gameSrc.begin(), gameSrc.end());

    int build_total = (int)rtSrc.size() + (int)gameSrc.size() + 2;
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "BUILD_TOTAL=%d\n", build_total);
        live_log(buf);
        log += buf;
    }

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
            rt_rsp << "d3d11.lib\nd3d10.lib\ndxgi.lib\nuser32.lib\ngdi32.lib\nwinmm.lib\nshell32.lib\n";
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
            f << "@echo on\n";
            f << "echo === PPSX33 MSVC build ===\n";
            f << "echo BUILD_TOTAL=" << build_total << "\n";
            f << "echo cwd=%CD%\n";
            f << "set \"VSWHERE=%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe\"\n";
            f << "set \"VSINSTALL=\"\n";
            f << "if exist \"%VSWHERE%\" for /f \"usebackq delims=\" %%i in (`\"%VSWHERE%\" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set \"VSINSTALL=%%i\"\n";
            if (!vcvars.empty()) {
                f << "if defined VSINSTALL (\n";
                f << "  echo vswhere install=%VSINSTALL%\n";
                f << "  call \"%VSINSTALL%\\VC\\Auxiliary\\Build\\vcvars64.bat\"\n";
                f << ") else (\n";
                f << "  echo vcvars=" << vcvars << "\n";
                f << "  call " << q(vcvars) << "\n";
                f << ")\n";
            } else {
                f << "if defined VSINSTALL call \"%VSINSTALL%\\VC\\Auxiliary\\Build\\vcvars64.bat\"\n";
                f << "if not defined VSINSTALL echo WARNING: no vswhere/vcvars — using PATH/bundled cl\n";
            }
            f << "echo post-vcvars OK\n";
            std::string cl = bundled_cl.empty() ? "cl" : q(bundled_cl);
            f << "echo cl=" << cl << "\n";
            f << "where cl 2>nul\n";
            f << "where cl >nul 2>&1\n";
            f << "if errorlevel 1 (\n";
            f << "  echo ERROR: cl.exe not on PATH after vcvars\n";
            f << "  exit /b 1\n";
            f << ")\n";
            f << "if not exist src\\ps3rt.cpp (echo MISSING src\\ps3rt.cpp & exit /b 1)\n";
            f << "if not exist obj mkdir obj\n";
            std::string cflags = "/nologo /O2 /EHsc /std:c++17 /bigobj /FS /MP /I src";
            f << "echo COMPILE runtime (parallel /MP)\n";
            f << cl << " " << cflags << " /DPS3RT_BUILD_DLL=1 /c";
            for (auto& n : rtSrc) f << " src\\" << n;
            f << " /Foobj\\ || exit /b 1\n";
            for (auto& n : rtSrc) {
                std::string base = n.substr(0, n.size() - 4);
                f << "if exist obj\\" << base << ".obj move /Y obj\\" << base << ".obj obj\\rt_" << n << ".obj >nul\n";
            }
            f << "echo LINK ps3rt.dll\n";
            f << "link @link_rt.rsp || exit /b 1\n";
            f << "echo COMPILE game chunks (parallel /MP)\n";
            f << cl << " " << cflags << " /c";
            for (auto& n : gameSrc) f << " src\\" << n;
            if (fs::exists(src / "game_main.cpp") &&
                std::find(gameSrc.begin(), gameSrc.end(), "game_main.cpp") == gameSrc.end()) {
                f << " src\\game_main.cpp";
            }
            f << " /Foobj\\ || exit /b 1\n";
            for (auto& n : gameSrc) {
                std::string base = n.substr(0, n.size() - 4);
                f << "if exist obj\\" << base << ".obj move /Y obj\\" << base << ".obj obj\\g_" << n << ".obj >nul\n";
            }
            if (fs::exists(src / "game_main.cpp") &&
                std::find(gameSrc.begin(), gameSrc.end(), "game_main.cpp") == gameSrc.end()) {
                f << "if exist obj\\game_main.obj move /Y obj\\game_main.obj obj\\g_game_main.obj >nul\n";
            }
            f << "echo LINK " << out_exe << "\n";
            f << "link @link_game.rsp || exit /b 1\n";
            f << "echo BUILD_OK\n";
            f << "exit /b 0\n";
        }

        log += "MSVC: out=" + out.string() + "\n";
        live_log("MSVC: out=" + out.string() + "\n");
        auto prev = fs::current_path(ec);
        fs::current_path(out, ec);
        int rc = run_cmd("cmd /c \"build_msvc.bat 2>&1\"", log);
        fs::current_path(prev, ec);
        if (rc != 0) {
            if (rc == 130 || g_build_cancel_requested.load()) {
                err = "BUILD_CANCELLED";
                live_close();
                return false;
            }
            std::string tail = log;
            if (tail.size() > 4000) tail = tail.substr(tail.size() - 4000);
            err = "MSVC build failed (exit=" + std::to_string(rc) + "):\n" + tail;
            live_log(err + "\n"); live_close();
            return false;
        }
        fs::remove(out / "build_msvc.bat", ec);
        cleanup_after_build(project_dir, out, log);
        live_log("Build OK (MSVC)\n");
        live_close();
        return true;
    }
#endif
    err = "No MSVC toolchain found";
    live_close();
    return false;
}

} // namespace ps3
