// Game asset host I/O: cellFs path mapping + PSARC TOC + virtual open + .rpf
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#ifndef _WIN32
#include <unistd.h>
#endif

static uint8_t* mem() { return ps3rt_memory(); }
static size_t mem_size() { return 512ull * 1024 * 1024; }

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static std::mutex g_fs_mu;
static char g_game_root[512] = {};

static void ensure_game_root() {
    if (g_game_root[0]) return;
#ifdef _WIN32
    char mod[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, mod, MAX_PATH)) {
        char* slash = strrchr(mod, '\\');
        if (slash) *slash = 0;
        std::snprintf(g_game_root, sizeof(g_game_root), "%s", mod);
    } else {
        GetCurrentDirectoryA((DWORD)sizeof(g_game_root), g_game_root);
    }
#else
    if (!getcwd(g_game_root, sizeof(g_game_root)))
        std::snprintf(g_game_root, sizeof(g_game_root), ".");
#endif
    ps3rt_log("[fs] game_root=%s\n", g_game_root);
}

PS3RT_API void ps3rt_set_game_root(const char* path) {
    std::lock_guard<std::mutex> lk(g_fs_mu);
    if (path && path[0])
        std::snprintf(g_game_root, sizeof(g_game_root), "%s", path);
    else
        g_game_root[0] = 0;
}

static bool map_guest_path(const char* guest, char* host, size_t host_len) {
    ensure_game_root();
    if (!guest || !host || host_len < 8) return false;
    while (*guest == ' ') ++guest;
    const char* p = guest;
    const char* prefixes[] = {
        "/dev_hdd0/game/", "/dev_hdd0/game",
        "/dev_hdd0/", "/dev_hdd0",
        "/app_home/", "/app_home",
        "/dev_bdvd/PS3_GAME/USRDIR/", "/dev_bdvd/PS3_GAME/USRDIR",
        "/dev_bdvd/PS3_GAME/", "/dev_bdvd/PS3_GAME",
        "/dev_bdvd/", "/dev_bdvd",
        "/host_root/", "/host_root",
        "/USRDIR/", "/USRDIR",
        nullptr
    };
    for (int i = 0; prefixes[i]; ++i) {
        size_t n = std::strlen(prefixes[i]);
        if (std::strncmp(p, prefixes[i], n) == 0) {
            p += n;
            while (*p == '/') ++p;
            break;
        }
    }
    if (p[0] == '.' && (p[1] == '/' || p[1] == '\\')) p += 2;
    while (*p == '/') ++p;
#ifdef _WIN32
    std::snprintf(host, host_len, "%s\\%s", g_game_root, p);
    for (char* q = host; *q; ++q) if (*q == '/') *q = '\\';
#else
    std::snprintf(host, host_len, "%s/%s", g_game_root, p);
#endif
    return true;
}

struct HostFd {
    FILE* f = nullptr;
    bool used = false;
    char path[512] = {};
    bool is_psarc = false;
    uint64_t arc_off = 0;
    uint64_t arc_len = 0;
    uint64_t pos = 0;
};
static HostFd g_fds[64];

static int fd_alloc(FILE* f, const char* path) {
    for (int i = 3; i < 64; ++i) {
        if (!g_fds[i].used) {
            g_fds[i] = HostFd{};
            g_fds[i].used = true;
            g_fds[i].f = f;
            std::snprintf(g_fds[i].path, sizeof(g_fds[i].path), "%s", path ? path : "");
            return i;
        }
    }
    return -1;
}

static HostFd* fd_get(int fd) {
    if (fd < 0 || fd >= 64 || !g_fds[fd].used) return nullptr;
    return &g_fds[fd];
}

struct PsarcTocEntry {
    std::string name;
    uint64_t offset = 0;
    uint64_t length = 0;
};

static uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}

struct PsarcArchive {
    std::string path;
    bool zlib = true;
    uint32_t block_size = 65536;
    std::vector<PsarcTocEntry> entries;
};

static std::vector<PsarcArchive> g_psarcs;

static bool psarc_load_toc(const char* path, PsarcArchive& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    uint8_t hdr[32];
    if (std::fread(hdr, 1, 32, f) != 32) { std::fclose(f); return false; }
    if (std::memcmp(hdr, "PSAR", 4) != 0) { std::fclose(f); return false; }
    uint32_t toc_len = be32(hdr + 12);
    uint32_t toc_entsz = be32(hdr + 16);
    uint32_t toc_ents = be32(hdr + 20);
    out.block_size = be32(hdr + 24);
    out.zlib = (std::memcmp(hdr + 8, "zlib", 4) == 0);
    out.path = path;
    if (toc_len == 0 || toc_ents == 0 || toc_ents > 200000 || toc_entsz < 30) {
        std::fclose(f); return false;
    }
    std::vector<uint8_t> toc(toc_len);
    if (std::fread(toc.data(), 1, toc_len, f) != toc_len) { std::fclose(f); return false; }
    out.entries.clear();
    for (size_t i = 0; i + 8 < toc.size(); ++i) {
        if (toc[i] == '/' || (toc[i] >= 'a' && toc[i] <= 'z') || (toc[i] >= 'A' && toc[i] <= 'Z')) {
            size_t j = i;
            std::string name;
            while (j < toc.size() && toc[j] >= 32 && toc[j] < 127 && toc[j] != '\\') {
                name.push_back((char)toc[j++]);
                if (name.size() > 240) break;
            }
            if (name.size() >= 4 && name.find('.') != std::string::npos) {
                while (!name.empty() && name[0] == '/') name.erase(name.begin());
                PsarcTocEntry e;
                e.name = name;
                out.entries.push_back(std::move(e));
                i = j;
            }
        }
    }
    std::sort(out.entries.begin(), out.entries.end(),
              [](const PsarcTocEntry& a, const PsarcTocEntry& b) { return a.name < b.name; });
    out.entries.erase(std::unique(out.entries.begin(), out.entries.end(),
                                  [](const PsarcTocEntry& a, const PsarcTocEntry& b) { return a.name == b.name; }),
                      out.entries.end());
    std::fclose(f);
    ps3rt_log("[psarc] %s toc_entries_hdr=%u names_found=%zu zlib=%d\n",
              path, toc_ents, out.entries.size(), out.zlib ? 1 : 0);
    return true;
}

static void scan_one_pattern(const char* pattern) {
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    char dir[520] = {};
    const char* slash = strrchr(pattern, '\\');
    if (slash) {
        size_t n = (size_t)(slash - pattern);
        if (n >= sizeof(dir)) n = sizeof(dir) - 1;
        std::memcpy(dir, pattern, n);
        dir[n] = 0;
    } else {
        std::snprintf(dir, sizeof(dir), "%s", g_game_root);
    }
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char full[560];
        std::snprintf(full, sizeof(full), "%s\\%s", dir, fd.cFileName);
        const char* name = fd.cFileName;
        size_t len = std::strlen(name);
        if (len >= 6 && _stricmp(name + len - 6, ".psarc") == 0) {
            PsarcArchive a;
            if (psarc_load_toc(full, a))
                g_psarcs.push_back(std::move(a));
        } else if (len >= 4 && _stricmp(name + len - 4, ".rpf") == 0) {
            ps3rt_log("[rpf] found %s\n", full);
        } else if (_stricmp(name, "EBOOT.BIN") == 0 || _stricmp(name, "PARAM.SFO") == 0) {
            ps3rt_log("[asset] found %s\n", full);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    (void)pattern;
#endif
}

// Recursive shallow scan for .psarc under a root (max depth 3)
static void scan_psarc_recursive(const char* root, int depth) {
#ifdef _WIN32
    if (!root || !*root || depth > 3) return;
    char pattern[560];
    std::snprintf(pattern, sizeof(pattern), "%s\\*", root);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.cFileName[0] == '.' && (fd.cFileName[1] == 0 || (fd.cFileName[1] == '.' && fd.cFileName[2] == 0)))
            continue;
        char full[560];
        std::snprintf(full, sizeof(full), "%s\\%s", root, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_psarc_recursive(full, depth + 1);
        } else {
            size_t len = std::strlen(fd.cFileName);
            if (len >= 6 && _stricmp(fd.cFileName + len - 6, ".psarc") == 0) {
                PsarcArchive a;
                if (psarc_load_toc(full, a))
                    g_psarcs.push_back(std::move(a));
            } else if (len >= 4 && _stricmp(fd.cFileName + len - 4, ".rpf") == 0) {
                ps3rt_log("[rpf] found %s\n", full);
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    (void)root; (void)depth;
#endif
}

static void psarc_scan_game_root() {
    ensure_game_root();
    g_psarcs.clear();
#ifdef _WIN32
    char pattern[560];
    // When EXE is in USRDIR (next to EBOOT.BIN), scan here first
    const char* globs[] = {
        "%s\\*.psarc",
        "%s\\*.rpf",
        "%s\\data\\*.psarc",
        "%s\\DATA\\*.psarc",
        "%s\\USRDIR\\*.psarc",
        "%s\\PS3_GAME\\USRDIR\\*.psarc",
        nullptr
    };
    for (int i = 0; globs[i]; ++i) {
        std::snprintf(pattern, sizeof(pattern), globs[i], g_game_root);
        scan_one_pattern(pattern);
    }
    // Shallow recursive scan under game_root (finds nested packs)
    scan_psarc_recursive(g_game_root, 0);

    // If game_root is USRDIR, also scan parent PS3_GAME and title root lightly
    char parent[520] = {};
    std::snprintf(parent, sizeof(parent), "%s", g_game_root);
    char* slash = strrchr(parent, '\\');
    if (slash) {
        *slash = 0;
        scan_one_pattern((std::snprintf(pattern, sizeof(pattern), "%s\\*.psarc", parent), pattern));
        // If parent is PS3_GAME, go one more level up for title root assets
        const char* base = strrchr(parent, '\\');
        const char* leaf = base ? base + 1 : parent;
        if (leaf && _stricmp(leaf, "PS3_GAME") == 0) {
            char title[520];
            std::snprintf(title, sizeof(title), "%s", parent);
            char* s2 = strrchr(title, '\\');
            if (s2) {
                *s2 = 0;
                scan_one_pattern((std::snprintf(pattern, sizeof(pattern), "%s\\*.psarc", title), pattern));
            }
        }
    }

    ps3rt_log("[fs] asset scan done root=%s  psarcs=%zu\n", g_game_root, g_psarcs.size());
#endif
}

static int fs_open(PPUContext* c) {
    uint64_t path_ea = c->gpr[3];
    uint64_t fd_ea = c->gpr[5] ? c->gpr[5] : c->gpr[6];
    if (!path_ea || path_ea >= mem_size()) { c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
    char guest[512] = {};
    for (size_t i = 0; i < sizeof(guest) - 1; ++i) {
        if (path_ea + i >= mem_size()) break;
        char ch = (char)mem()[path_ea + i];
        guest[i] = ch;
        if (!ch) break;
    }
    char host[768];
    if (!map_guest_path(guest, host, sizeof(host))) {
        c->gpr[3] = (uint64_t)(int32_t)-1; return 0;
    }
    uint32_t flags = (uint32_t)c->gpr[4];
    const char* mode = "rb";
    if (flags & 2) mode = "r+b";
    else if (flags & 1) mode = "wb";

    FILE* f = std::fopen(host, mode);
    if (!f && (flags & 1)) f = std::fopen(host, "wb");
    int fd = -1;
    if (f) {
        fd = fd_alloc(f, host);
        if (fd < 0) { std::fclose(f); c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
    } else {
        std::string key = guest;
        const char* slash = strrchr(host, '\\');
        if (!slash) slash = strrchr(host, '/');
        if (slash && slash[1]) key = slash + 1;
        for (auto& arc : g_psarcs) {
            for (auto& e : arc.entries) {
                if (e.name.empty()) continue;
                bool match = false;
#ifdef _WIN32
                if (_stricmp(e.name.c_str(), key.c_str()) == 0) match = true;
                else {
                    size_t el = e.name.size(), kl = key.size();
                    if (el > kl && _stricmp(e.name.c_str() + el - kl, key.c_str()) == 0)
                        match = true;
                }
#else
                match = (e.name == key);
#endif
                if (!match) continue;
                FILE* af = std::fopen(arc.path.c_str(), "rb");
                if (!af) continue;
                fd = fd_alloc(af, e.name.c_str());
                if (fd < 0) { std::fclose(af); continue; }
                g_fds[fd].is_psarc = true;
                g_fds[fd].arc_off = e.offset;
                g_fds[fd].arc_len = e.length ? e.length : 0x100000;
                g_fds[fd].pos = 0;
                if (e.offset) std::fseek(af, (long)e.offset, SEEK_SET);
                ps3rt_log("[psarc] virtual open %s from %s\n", e.name.c_str(), arc.path.c_str());
                break;
            }
            if (fd >= 0) break;
        }
    }
    static int s_log = 0;
    if (s_log < 64) {
        ps3rt_log("[fs] open guest=\"%s\" host=\"%s\" %s\n", guest, host, fd >= 0 ? "OK" : "FAIL");
        ++s_log;
    }
    if (fd < 0) { c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
    if (fd_ea && fd_ea + 4 <= mem_size()) {
        mem()[fd_ea + 0] = 0;
        mem()[fd_ea + 1] = 0;
        mem()[fd_ea + 2] = (uint8_t)((fd >> 8) & 0xFF);
        mem()[fd_ea + 3] = (uint8_t)(fd & 0xFF);
    }
    c->gpr[3] = 0;
    return 0;
}

static int fs_read(PPUContext* c) {
    int fd = (int)(uint32_t)c->gpr[3];
    uint64_t buf = c->gpr[4];
    uint64_t size = c->gpr[5];
    uint64_t nread_ea = c->gpr[6];
    HostFd* h = fd_get(fd);
    if (!h || !h->f || !buf || buf + size > mem_size()) {
        c->gpr[3] = (uint64_t)(int32_t)-1; return 0;
    }
    size_t n = std::fread(mem() + buf, 1, (size_t)size, h->f);
    if (h->is_psarc) h->pos += n;
    if (nread_ea && nread_ea + 8 <= mem_size()) {
        uint64_t be = n;
        for (int i = 7; i >= 0; --i) { mem()[nread_ea + (7 - i)] = (uint8_t)((be >> (i * 8)) & 0xFF); }
    }
    c->gpr[3] = 0;
    return 0;
}

static int fs_write(PPUContext* c) {
    int fd = (int)(uint32_t)c->gpr[3];
    uint64_t buf = c->gpr[4];
    uint64_t size = c->gpr[5];
    uint64_t nwr_ea = c->gpr[6];
    HostFd* h = fd_get(fd);
    if (!h || !h->f || h->is_psarc || !buf || buf + size > mem_size()) {
        c->gpr[3] = (uint64_t)(int32_t)-1; return 0;
    }
    size_t n = std::fwrite(mem() + buf, 1, (size_t)size, h->f);
    if (nwr_ea && nwr_ea + 8 <= mem_size()) {
        uint64_t be = n;
        for (int i = 7; i >= 0; --i) { mem()[nwr_ea + (7 - i)] = (uint8_t)((be >> (i * 8)) & 0xFF); }
    }
    c->gpr[3] = 0;
    return 0;
}

static int fs_close(PPUContext* c) {
    int fd = (int)(uint32_t)c->gpr[3];
    HostFd* h = fd_get(fd);
    if (!h) { c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
    if (h->f) std::fclose(h->f);
    h->f = nullptr;
    h->used = false;
    c->gpr[3] = 0;
    return 0;
}

PS3RT_API int ps3rt_fs_dispatch(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    std::lock_guard<std::mutex> lk(g_fs_mu);
    static bool scanned = false;
    if (!scanned) { scanned = true; psarc_scan_game_root(); }

    switch (nid) {
    case 0x718BF5F8: return fs_open(c);   // cellFsOpen
    case 0x4D5B9EF2: return fs_read(c);   // cellFsRead
    case 0xECDCF2AB: return fs_write(c);  // cellFsWrite
    case 0x2CB51F0F: return fs_close(c);  // cellFsClose
    case 0xA397D042: // cellFsLseek
    case 0x7F13FC8C: // cellFsStat
    case 0xEF3EFA34: // cellFsFstat
        c->gpr[3] = 0;
        return 0;
    default:
        c->gpr[3] = 0;
        return 0;
    }
}

// Boot-time asset scan export (call early so log shows packs before first open)
PS3RT_API void ps3rt_assets_boot_scan(void) {
    std::lock_guard<std::mutex> lk(g_fs_mu);
    psarc_scan_game_root();
}
