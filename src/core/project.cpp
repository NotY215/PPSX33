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
    if (g_build_cancel_requested.load()) { log += "BUILD_CANCELLED\n"; return 130; }
    SECURITY_ATTRIBUTES sa{}; sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE;
    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return -1;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    HANDLE job = CreateJobObjectA(nullptr, nullptr);
    if (job) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));
    }
    STARTUPINFOA si{}; si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe; si.hStdError = writePipe; si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::string full = "cmd.exe /S /C \\"" + cmd + " 2>&1\\"";
    std::vector<char> mutableCmd(full.begin(), full.end()); mutableCmd.push_back('\0');
    BOOL created = CreateProcessA(nullptr, mutableCmd.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &si, &pi);
    CloseHandle(writePipe);
    if (!created) {
        CloseHandle(readPipe); if (job) CloseHandle(job); return -1;
    }
    if (job) AssignProcessToJobObject(job, pi.hProcess);
    ResumeThread(pi.hThread);
    char buf[1024];
    bool cancelled = false;
    for (;;) {
        DWORD available = 0;
        while (PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr) && available) {
            DWORD got = 0;
            if (!ReadFile(readPipe, buf, (DWORD)std::min<size_t>(sizeof(buf), available), &got, nullptr) || got == 0) break;
            log.append(buf, got);
            available = 0;
            if (!PeekNamedPipe(readPipe, nullptr, 0, nullptr, &available, nullptr)) break;
        }
        if (g_build_cancel_requested.load()) {
            cancelled = true;
            if (job) TerminateJobObject(job, 130);
            else TerminateProcess(pi.hProcess, 130);
        }
        if (WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0) {
            DWORD availableAfter = 0;
            while (PeekNamedPipe(readPipe, nullptr, 0, nullptr, &availableAfter, nullptr) && availableAfter) {
                DWORD got = 0;
                if (!ReadFile(readPipe, buf, (DWORD)std::min<size_t>(sizeof(buf), availableAfter), &got, nullptr) || !got) break;
                log.append(buf, got);
            }
            break;
        }
        if (cancelled) WaitForSingleObject(pi.hProcess, 5000);
    }
    DWORD exitCode = 1; GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(readPipe);
    if (job) CloseHandle(job);
    if (cancelled || g_build_cancel_requested.load()) { log += "\nBUILD_CANCELLED\n"; return 130; }
    return (int)exitCode;
#else
    FILE* p = popen((cmd + " 2>&1").c_str(), "r");
    if (!p) return -1;
    char buf[512];
    while (std::fgets(buf, sizeof buf, p)) log += buf;
    return pclose(p);
#endif
}

std::string find_vcvars64() {
#ifdef _WIN32
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
                if (fs::exists(vcvars)) { _pclose(p); return vcvars.string(); }
            }
            _pclose(p);
        }
    }
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

std::string find_bundled_cl(const fs::path& compilers_dir) {
    const char* subdirs[] = { "MSCV", "MSVC", "msvc", "mscv", "" };
    for (auto sub : subdirs) {
        fs::path p = sub[0] ? (compilers_dir / sub / "cl.exe") : (compilers_dir / "cl.exe");
        if (fs::exists(p)) return p.string();
    }
    return {};
}

std::string find_tool(const fs::path& dir, const std::vector<std::string>& names) {
    for (const auto& n : names) {
        fs::path p = dir / (n + (std::string(kExe) == ".exe" ? ".exe" : ""));
        if (!fs::exists(p)) continue;
        if (n == "g++" || n == "c++" || n == "gcc") {
            if (!fs::exists(dir / "libexec")) continue;
        }
        return p.string();
    }
    return names.front();
}

void cleanup_after_build(const fs::path& project_dir, const fs::path& out, std::string& log) {
    std::error_code ec;
    fs::path codebase = project_dir / "codebase";
    fs::path input = project_dir / "input";
    fs::path obj = out / "obj";
    for (const char* name : { "lift_report.txt", "analysis_report.txt" }) {
        fs::path src = codebase / name;
        if (fs::exists(src)) fs::copy_file(src, out / name, fs::copy_options::overwrite_existing, ec);
    }
    fs::path bat = out / "build_msvc.bat";
    if (fs::exists(bat)) fs::remove(bat, ec);
    fs::path ninja = out / "build.ninja";
    if (fs::exists(ninja)) fs::remove(ninja, ec);
}

} // namespace

void begin_build() { g_build_cancel_requested.store(false); }
void cancel_build() { g_build_cancel_requested.store(true); }

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err) {
    fs::path out = fs::path(project_dir) / "output";
    fs::path src = out / "src";
    fs::path code = fs::path(project_dir) / "codebase";
    std::error_code ec;
    fs::create_directories(src, ec);
    fs::create_directories(out, ec);

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
        } else if (n == "ppu_chunks.cpp" || n == "image_info.h" || n == "ppu_runtime.h" || n == "nid_table.h") {
            fs::copy_file(e.path(), src / n, ow, ec);
        }
    }
    // Always ensure image_info.h is present (required by game_main.cpp)
    if (fs::exists(code / "image_info.h") && !fs::exists(src / "image_info.h"))
        fs::copy_file(code / "image_info.h", src / "image_info.h", ow, ec);
    if (fs::exists(code / "lift_report.txt"))
        fs::copy_file(code / "lift_report.txt", out / "lift_report.txt", ow, ec);
    if (fs::exists(code / "analysis_report.txt"))
        fs::copy_file(code / "analysis_report.txt", out / "analysis_report.txt", ow, ec);
    fs::copy_file(code / "guest_image.bin", out / "guest_image.bin", ow, ec);

    // CRITICAL: copy runtime headers + sources (Phase F split headers optional)
    for (const char* n : {"ppu_runtime.h", "nid_table.h", "ps3rt.cpp", "spu_stub.cpp", "rsx_stub.cpp"}) {
        fs::path from = fs::path(runtime_dir) / n;
        if (!fs::exists(from)) { err = "Runtime file missing: " + from.string(); return false; }
        fs::copy_file(from, src / n, ow, ec);
        if (std::string(n).find(".cpp") != std::string::npos) rtSrc.push_back(n);
    }
    // Phase F split headers (if present alongside ppu_runtime.h)
    for (const char* n : {"ppu_runtime_core.h", "ppu_runtime_core_a.h",
                          "ppu_runtime_core_b.h", "ppu_runtime_run.h"}) {
        fs::path from = fs::path(runtime_dir) / n;
        if (fs::exists(from))
            fs::copy_file(from, src / n, ow, ec);
    }
    fs::create_directories(out / "obj", ec);

    // Derive output EXE name from project folder name
    std::string out_exe = fs::path(project_dir).filename().string() + kExe;

#ifdef _WIN32
    std::string vcvars = find_vcvars64();
    std::string bundled_cl = find_bundled_cl(compilers_dir);
    bool use_msvc = !vcvars.empty() || !bundled_cl.empty();

    if (use_msvc) {
        fs::path bat = out / "build_msvc.bat";
        {
            std::ofstream f(bat);
            f << "@echo off\nsetlocal\n";
            if (!vcvars.empty()) {
                f << "call " << q(vcvars) << " >nul 2>&1\n";
            }
            std::string cl = bundled_cl.empty() ? "cl" : q(bundled_cl);
            std::string link_cmd = "link";
            std::string cflags = "/nologo /O2 /EHsc /std:c++17 /I src /DPS3RT_BUILD_DLL=1";
            std::string rt_objs, game_objs;
            for (auto& n : rtSrc) {
                std::string o = "obj\\rt_" + n + ".obj";
                f << cl << " " << cflags << " /c src\\" << n << " /Fo" << o << " || exit /b 1\n";
                rt_objs += " " + o;
            }
            f << link_cmd << " /nologo /DLL /OUT:ps3rt.dll" << rt_objs
              << " d3d11.lib d3d10.lib dxgi.lib user32.lib gdi32.lib || exit /b 1\n";
            for (auto& n : gameSrc) {
                std::string o = "obj\\g_" + n + ".obj";
                f << cl << " /nologo /O2 /EHsc /std:c++17 /I src /c src\\" << n << " /Fo" << o << " || exit /b 1\n";
                game_objs += " " + o;
            }
            // Also compile any leftover game_main / ppu_chunks if listed
            if (fs::exists(src / "game_main.cpp") &&
                std::find(gameSrc.begin(), gameSrc.end(), "game_main.cpp") == gameSrc.end()) {
                f << cl << " /nologo /O2 /EHsc /std:c++17 /I src /c src\\game_main.cpp /Foobj\\g_game_main.obj || exit /b 1\n";
                game_objs += " obj\\g_game_main.obj";
            }
            if (fs::exists(src / "ppu_chunks.cpp") &&
                std::find(gameSrc.begin(), gameSrc.end(), "ppu_chunks.cpp") == gameSrc.end()) {
                f << cl << " /nologo /O2 /EHsc /std:c++17 /I src /c src\\ppu_chunks.cpp /Foobj\\g_ppu_chunks.obj || exit /b 1\n";
                game_objs += " obj\\g_ppu_chunks.obj";
            }
            f << link_cmd << " /nologo /OUT:" << out_exe << game_objs << " ps3rt.lib || exit /b 1\n"
              << "echo Build OK\n";
        }
        auto prev = fs::current_path(ec);
        fs::current_path(out, ec);
        int rc = run("cmd /c build_msvc.bat", log);
        fs::current_path(prev, ec);
        if (rc != 0) { err = (rc == 130 || g_build_cancel_requested.load()) ? "BUILD_CANCELLED" : "MSVC build failed. See log."; return false; }
        log += "Build OK: output/" + out_exe + " + output/ps3rt.dll + output/guest_image.bin (MSVC)\n";
        cleanup_after_build(project_dir, out, log);
        return true;
    }
#endif

    fs::path cdir(compilers_dir);
    std::string cxx = find_tool(cdir, {"g++", "c++"});
    std::string ninja = find_tool(cdir, {"ninja"});
    log += "MSVC not found. Falling back to g++.\n";

    std::vector<Step> steps;
    std::string cflags = "-O2 -std=c++17 -I src";
    std::vector<std::string> gameObjs, rtObjs;
    for (auto& n : rtSrc) {
        std::string o = "obj/rt_" + n + ".o";
        steps.push_back({{ "src/" + n }, o, q(cxx) + " " + cflags + " -DPS3RT_BUILD_DLL=1 -fPIC -c src/" + n + " -o " + o});
        rtObjs.push_back(o);
    }
    std::string dll = std::string("ps3rt") + kDll;
    {
        std::string objs; for (auto& o : rtObjs) objs += " " + o;
        steps.push_back({rtObjs, dll, q(cxx) + " -shared" + objs + " -o " + dll});
    }
    for (auto& n : gameSrc) {
        std::string o = "obj/g_" + n + ".o";
        steps.push_back({{ "src/" + n }, o, q(cxx) + " " + cflags + " -c src/" + n + " -o " + o});
        gameObjs.push_back(o);
    }
    {
        std::string objs; for (auto& o : gameObjs) objs += " " + o;
        std::string rpath;
#ifndef _WIN32
        rpath = " -Wl,-rpath,'$ORIGIN'";
#endif
        std::string exe = out_exe;
        std::vector<std::string> ins = gameObjs; ins.push_back(dll);
        steps.push_back({ins, exe, q(cxx) + objs + " ./" + dll + rpath + " -o " + exe});
    }

    {
        std::ofstream n(out / "build.ninja");
        n << "# GENERATED (g++ fallback)\nrule run\n  command = $cmd\n  description = $out\n\n";
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
    if (fs::exists(ninja)) rc = run(q(ninja), log);
    else {
        log += "(ninja not found, compiling sequentially)\n";
        for (auto& s : steps) { rc = run(s.cmd, log); if (rc != 0) break; }
    }
    fs::current_path(prev, ec);
    if (rc != 0) { err = (rc == 130 || g_build_cancel_requested.load()) ? "BUILD_CANCELLED" : "Build failed. See log."; return false; }
    log += "Build OK: output/" + out_exe + " + output/" + std::string("ps3rt") + kDll + " (g++)\n";
    cleanup_after_build(project_dir, out, log);
    return true;
}

} // namespace ps3
