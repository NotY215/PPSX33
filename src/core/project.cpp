#include "project.h"
#include "elf_loader.h"
#include "ppu_lifter.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <atomic>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

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
    fs::path code = fs::path(project_dir) / "codebase";
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

    std::ostringstream o;
    o << "ELF loaded: " << img.segments.size() << " segments, entry OPD 0x" << std::hex << img.entry << std::dec << "\n"
      << "Analysis: symbols=" << img.symbols.size()
      << " opds=" << img.opds.size()
      << " prx_hits=" << img.prx_imports.size()
      << " spu_images=" << img.spu_images.size() << "\n"
      << "Instructions: " << st.instructions << "  translated: " << st.implemented
      << "  unimplemented: " << st.unimplemented << "  chunks: " << st.chunks << "\n"
      << "See codebase/lift_report.txt and codebase/analysis_report.txt.\n";
    log = o.str();
    return true;
}

namespace {
std::atomic<bool> g_build_cancel_requested{false};
FILE* g_build_live = nullptr;
void live_log(const std::string& s) {
    if (!g_build_live || s.empty()) return;
    std::fwrite(s.data(), 1, s.size(), g_build_live);
    std::fflush(g_build_live);
}
void live_close() {
    if (g_build_live) { std::fclose(g_build_live); g_build_live = nullptr; }
}
#ifdef _WIN32
const char* kExe = ".exe"; const char* kDll = ".dll";
#else
const char* kExe = "";     const char* kDll = ".so";
#endif

std::string q(const std::string& s) { return "\"" + s + "\""; }

std::string find_vcvars64() {
#ifdef _WIN32
    const char* candidates[] = {
        "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
    };
    for (auto c : candidates) if (fs::exists(c)) return c;
#endif
    return {};
}

std::string find_bundled_cl(const std::string& compilers_dir) {
#ifdef _WIN32
    fs::path p = fs::path(compilers_dir) / "MSCV" / "cl.exe";
    if (fs::exists(p)) return p.string();
#endif
    return {};
}

int run_cmd(const std::string& cmd, std::string& log) {
    log += "> " + cmd + "\n"; live_log("> " + cmd + "\n");
#ifdef _WIN32
    if (g_build_cancel_requested.load()) { log += "BUILD_CANCELLED\n"; return 130; }
    std::string full = cmd + " 2>&1";
    FILE* pipe = _popen(full.c_str(), "r");
#else
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
#endif
    if (!pipe) { log += "failed to start command\n"; return -1; }
    char buf[4096];
    while (std::fgets(buf, sizeof(buf), pipe)) {
        size_t got = std::strlen(buf);
        log.append(buf, got); live_log(std::string(buf, got));
        if (g_build_cancel_requested.load()) break;
    }
#ifdef _WIN32
    int rc = _pclose(pipe);
#else
    int rc = pclose(pipe);
#endif
    if (g_build_cancel_requested.load()) { log += "\nBUILD_CANCELLED\n"; return 130; }
    return rc;
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
        log += "cleanup: trimmed codebase/ (kept reports)\n";
        live_log("cleanup: trimmed codebase/\n");
    }
    if (fs::exists(input)) {
        for (auto& e : fs::directory_iterator(input, ec)) {
            std::string n = e.path().filename().string();
            if (n == "EBOOT.elf" || n == "EBOOT.BIN" || n == "EBOOT.bin") continue;
            fs::remove_all(e.path(), ec);
        }
        log += "cleanup: trimmed input/ (kept EBOOT)\n";
        live_log("cleanup: trimmed input/\n");
    }
    log += "cleanup: done (exe/dll/guest_image.bin kept)\n";
    live_log("cleanup: done\n");
}
} // anon

void begin_build() { g_build_cancel_requested.store(false); }
void cancel_build() { g_build_cancel_requested.store(true); }

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err) {
    std::error_code ec;
    fs::path out = fs::path(project_dir) / "output";
    fs::path src = out / "src";
    fs::path code = fs::path(project_dir) / "codebase";
    fs::create_directories(out, ec);
    fs::create_directories(src, ec);

    live_close();
    g_build_live = std::fopen((out / "build_live.log").string().c_str(), "w");
    live_log("PPSX33 build started\n");

    if (!fs::exists(code / "ppu_chunks.cpp")) {
        err = "codebase/ppu_chunks.cpp missing — run Decompile first.";
        return false;
    }

    const fs::copy_options ow = fs::copy_options::overwrite_existing;
    std::vector<std::string> gameSrc, rtSrc;

    for (auto& e : fs::directory_iterator(code)) {
        std::string n = e.path().filename().string();
        if (n.rfind("ppu_chunk", 0) == 0 || n == "game_main.cpp") {
            fs::copy_file(e.path(), src / n, ow, ec);
            if (e.path().extension() == ".cpp") gameSrc.push_back(n);
        } else if (n == "image_info.h" || n == "ppu_runtime.h" || n == "nid_table.h") {
            fs::copy_file(e.path(), src / n, ow, ec);
        }
    }
    if (fs::exists(code / "image_info.h"))
        fs::copy_file(code / "image_info.h", src / "image_info.h", ow, ec);
    else if (!fs::exists(src / "image_info.h")) {
        std::ofstream hi(src / "image_info.h");
        hi << "#pragma once\n#include <cstdint>\n"
           << "static const uint64_t kEntryPc = 0x10230ull;\n"
           << "static const uint64_t kEntryToc = 0;\n";
        log += "warning: image_info.h missing — wrote stub\n";
    }
    if (fs::exists(code / "lift_report.txt"))
        fs::copy_file(code / "lift_report.txt", out / "lift_report.txt", ow, ec);
    if (fs::exists(code / "analysis_report.txt"))
        fs::copy_file(code / "analysis_report.txt", out / "analysis_report.txt", ow, ec);
    fs::copy_file(code / "guest_image.bin", out / "guest_image.bin", ow, ec);

    for (const char* n : {"ppu_runtime.h", "nid_table.h", "ps3rt.cpp", "spu_stub.cpp", "rsx_stub.cpp"}) {
        fs::path from = fs::path(runtime_dir) / n;
        if (!fs::exists(from)) { err = "Runtime file missing: " + from.string(); return false; }
        fs::copy_file(from, src / n, ow, ec);
        if (std::string(n).find(".cpp") != std::string::npos) rtSrc.push_back(n);
    }
    for (const char* n : {"ppu_runtime_core.h", "ppu_runtime_core_a.h",
                          "ppu_runtime_core_b.h", "ppu_runtime_run.h", "ps3rt_stage.h"}) {
        fs::path from = fs::path(runtime_dir) / n;
        if (fs::exists(from))
            fs::copy_file(from, src / n, ow, ec);
    }
    fs::create_directories(out / "obj", ec);
    log += "src files: " + std::to_string(gameSrc.size()) + " game + "
         + std::to_string(rtSrc.size()) + " runtime cpp\n";
    if (gameSrc.empty()) {
        err = "No game sources (game_main/ppu_chunks) copied — re-run Decompile.";
        return false;
    }

    int build_total = 0;
    {
        std::error_code ec2;
        for (auto& e : fs::directory_iterator(src, ec2)) {
            if (e.path().extension() == ".cpp") ++build_total;
        }
        char buf[64];
        std::snprintf(buf, sizeof(buf), "BUILD_TOTAL=%d\n", build_total);
        live_log(buf);
        log += buf;
    }

    std::string out_exe = fs::path(project_dir).filename().string() + kExe;

#ifdef _WIN32
    std::string vcvars = find_vcvars64();
    std::string bundled_cl = find_bundled_cl(compilers_dir);
    bool use_msvc = !vcvars.empty() || !bundled_cl.empty();

    if (use_msvc) {
        // Response files avoid Windows 8191-char command-line limit (150+ objs)
        {
            std::ofstream rt_rsp(out / "link_rt.rsp");
            rt_rsp << "/nologo\n/DLL\n/OUT:ps3rt.dll\n";
            for (auto& n : rtSrc) rt_rsp << "obj\\rt_" << n << ".obj\n";
            rt_rsp << "d3d11.lib\nd3d10.lib\ndxgi.lib\nuser32.lib\ngdi32.lib\n";
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
            // Shrink PATH before vcvars — prevents "The input line is too long"
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
            f << "if not exist src\\game_main.cpp (echo MISSING src\\game_main.cpp & exit /b 1)\n";
            f << "if not exist src\\ppu_chunks.cpp (echo MISSING src\\ppu_chunks.cpp & exit /b 1)\n";
            f << "if not exist src\\image_info.h (echo MISSING src\\image_info.h & exit /b 1)\n";
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
        log += "MSVC: out=" + out.string() + "\n";
        live_log("MSVC: out=" + out.string() + "\n");
        auto prev = fs::current_path(ec);
        fs::current_path(out, ec);
        if (ec) {
            log += "chdir failed: " + ec.message() + "\n";
            live_log("chdir failed: " + ec.message() + "\n");
        }
        int rc = run_cmd("build_msvc.bat", log);
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
