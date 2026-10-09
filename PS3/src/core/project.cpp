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

std::string find_tool(const fs::path& dir, const std::vector<std::string>& names) {
    for (const auto& n : names) {
        fs::path p = dir / (n + (std::string(kExe) == ".exe" ? ".exe" : ""));
        if (fs::exists(p)) return p.string();
    }
    return names.front();   // fall back to PATH lookup
}

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
std::string q(const std::string& s) { return "\"" + s + "\""; }
}

bool build_project(const std::string& project_dir, const std::string& runtime_dir,
                   const std::string& compilers_dir, std::string& log, std::string& err) {
    fs::path out = fs::path(project_dir) / "output";
    fs::path code = fs::path(project_dir) / "codebase";
    fs::path src = out / "src";
    std::error_code ec;
    fs::create_directories(src, ec);
    if (!fs::exists(code / "ppu_chunks.cpp")) { err = "Run the lift step first (codebase is empty)."; return false; }

    // 1) gather sources into output/src
    const fs::copy_options ow = fs::copy_options::overwrite_existing;
    std::vector<std::string> gameSrc, rtSrc;
    for (auto& e : fs::directory_iterator(code)) {
        std::string n = e.path().filename().string();
        if (n.rfind("ppu_chunk", 0) == 0 || n == "game_main.cpp") { fs::copy_file(e.path(), src / n, ow, ec); if (e.path().extension() == ".cpp") gameSrc.push_back(n); }
        else if (n == "image_info.h") fs::copy_file(e.path(), src / n, ow, ec);
    }
    fs::copy_file(code / "guest_image.bin", out / "guest_image.bin", ow, ec);
    for (const char* n : {"ppu_runtime.h", "ps3rt.cpp", "spu_stub.cpp", "rsx_stub.cpp"}) {
        fs::path from = fs::path(runtime_dir) / n;
        if (!fs::exists(from)) { err = "Runtime file missing: " + from.string(); return false; }
        fs::copy_file(from, src / n, ow, ec);
        if (std::string(n).find(".cpp") != std::string::npos) rtSrc.push_back(n);
    }

    // 2) pick compiler / ninja
    fs::path cdir(compilers_dir);
    std::string cxx = find_tool(cdir, {"g++", "c++"});
    std::string ninja = find_tool(cdir, {"ninja"});

    // 3) steps
    std::vector<Step> steps;
    std::string cflags = "-O2 -std=c++17 -I src";
    std::vector<std::string> gameObjs, rtObjs;
    for (auto& n : rtSrc)   { std::string o = "obj/rt_" + n + ".o";   steps.push_back({{"src/" + n}, o, q(cxx) + " " + cflags + " -DPS3RT_BUILD_DLL=1 -fPIC -c src/" + n + " -o " + o}); rtObjs.push_back(o); }
    std::string dll = std::string("ps3rt") + kDll;
    {
        std::string objs; for (auto& o : rtObjs) objs += " " + o;
        steps.push_back({rtObjs, dll, q(cxx) + " -shared" + objs + " -o " + dll});
    }
    for (auto& n : gameSrc) { std::string o = "obj/g_" + n + ".o";    steps.push_back({{"src/" + n}, o, q(cxx) + " " + cflags + " -c src/" + n + " -o " + o}); gameObjs.push_back(o); }
    {
        std::string objs; for (auto& o : gameObjs) objs += " " + o;
        std::string rpath;
#ifndef _WIN32
        rpath = " -Wl,-rpath,'$ORIGIN'";
#endif
        std::string exe = std::string("game") + kExe;
        std::vector<std::string> ins = gameObjs; ins.push_back(dll);
        steps.push_back({ins, exe, q(cxx) + objs + " ./" + dll + rpath + " -o " + exe});
    }
    fs::create_directories(out / "obj", ec);

    // 4) always write build.ninja (useful for humans / other AIs), then run
    {
        std::ofstream n(out / "build.ninja");
        n << "# GENERATED by ps3core build driver\nrule run\n  command = $cmd\n  description = $out\n\n";
        for (auto& s : steps) {
            std::string c = s.cmd; std::string esc; for (char ch : c) { if (ch == '$') esc += "$$"; else esc += ch; }
            n << "build " << s.out << ": run";
            for (auto& i : s.in) n << " " << i;
            n << "\n  cmd = " << esc << "\n";
        }
    }
    auto prev = fs::current_path(ec);
    fs::current_path(out, ec);
    int rc = 0;
    std::string ninjaPath = ninja;
    bool haveNinja = fs::exists(ninja);
    if (haveNinja) rc = run(q(ninja), log);
    else { log += "(ninja not found in Compilers-files, compiling sequentially)\n"; for (auto& s : steps) { rc = run(s.cmd, log); if (rc != 0) break; } }
    fs::current_path(prev, ec);
    if (rc != 0) { err = "Build failed. See log."; return false; }
    log += "Build OK: output/game" + std::string(kExe) + " + output/" + dll + " + output/guest_image.bin\n";
    return true;
}

} // namespace ps3
