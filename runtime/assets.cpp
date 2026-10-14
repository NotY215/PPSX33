// XX-J: cellFs path mapping + full PSARC TOC + zlib block decompress streaming
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cstdint>
#ifndef _WIN32
#include <unistd.h>
#endif

static int ps3_stricmp(const char* a, const char* b) {
    if (!a || !b) return (a ? 1 : 0) - (b ? 1 : 0);
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
        ++a; ++b;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}


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

// ---------------- zlib (dynamic) ----------------
#ifdef _WIN32
typedef int (*inflateInit2_fn)(void* strm, int windowBits, const char* version, int stream_size);
typedef int (*inflate_fn)(void* strm, int flush);
typedef int (*inflateEnd_fn)(void* strm);

// Minimal z_stream layout compatible with zlib
struct ZStream {
    const uint8_t* next_in;
    unsigned avail_in;
    unsigned long total_in;
    uint8_t* next_out;
    unsigned avail_out;
    unsigned long total_out;
    const char* msg;
    void* state;
    void* zalloc;
    void* zfree;
    void* opaque;
    int data_type;
    unsigned long adler;
    unsigned long reserved;
};

static HMODULE g_zlib = nullptr;
static inflateInit2_fn p_inflateInit2_ = nullptr;
static inflate_fn p_inflate = nullptr;
static inflateEnd_fn p_inflateEnd = nullptr;
static int g_zlib_tried = 0;

static bool zlib_load() {
    if (g_zlib_tried) return p_inflate != nullptr;
    g_zlib_tried = 1;
    const char* names[] = { "zlib1.dll", "zlib.dll", "libzlib.dll", nullptr };
    for (int i = 0; names[i]; ++i) {
        g_zlib = LoadLibraryA(names[i]);
        if (g_zlib) break;
    }
    if (!g_zlib) {
        ps3rt_log("[psarc] XX-J zlib DLL not found — uncompressed / stored blocks only\n");
        return false;
    }
    p_inflateInit2_ = (inflateInit2_fn)GetProcAddress(g_zlib, "inflateInit2_");
    p_inflate = (inflate_fn)GetProcAddress(g_zlib, "inflate");
    p_inflateEnd = (inflateEnd_fn)GetProcAddress(g_zlib, "inflateEnd");
    if (!p_inflateInit2_ || !p_inflate || !p_inflateEnd) {
        ps3rt_log("[psarc] XX-J zlib exports missing\n");
        FreeLibrary(g_zlib); g_zlib = nullptr;
        return false;
    }
    ps3rt_log("[psarc] XX-J zlib loaded\n");
    return true;
}

static bool zlib_inflate_block(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_cap, size_t* out_len) {
    if (!out_len) return false;
    *out_len = 0;
    if (!src || !dst || src_len == 0 || dst_cap == 0) return false;
    // Stored block (no compression): sizes match
    if (src_len <= dst_cap && src_len == dst_cap) {
        std::memcpy(dst, src, src_len);
        *out_len = src_len;
        return true;
    }
    if (!zlib_load()) {
        // Best effort: copy min
        size_t n = src_len < dst_cap ? src_len : dst_cap;
        std::memcpy(dst, src, n);
        *out_len = n;
        return true;
    }
    ZStream zs{};
    zs.zalloc = zs.zfree = zs.opaque = nullptr;
    // 15+32 = zlib+gzip header auto
    if (p_inflateInit2_(&zs, 15 + 32, "1.2.11", (int)sizeof(ZStream)) != 0) {
        // try raw deflate
        zs = {};
        if (p_inflateInit2_(&zs, -15, "1.2.11", (int)sizeof(ZStream)) != 0)
            return false;
    }
    zs.next_in = src;
    zs.avail_in = (unsigned)src_len;
    zs.next_out = dst;
    zs.avail_out = (unsigned)dst_cap;
    int rc = p_inflate(&zs, 4 /*Z_FINISH*/);
    *out_len = (size_t)zs.total_out;
    p_inflateEnd(&zs);
    return (rc == 1 /*Z_STREAM_END*/ || rc == 0) && *out_len > 0;
}
#else
static bool zlib_inflate_block(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_cap, size_t* out_len) {
    if (!out_len) return false;
    size_t n = src_len < dst_cap ? src_len : dst_cap;
    std::memcpy(dst, src, n);
    *out_len = n;
    return true;
}
static bool zlib_load() { return false; }
#endif

// ---------------- PSARC ----------------
static uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
static uint64_t be40(const uint8_t* p) {
    return (uint64_t(p[0]) << 32) | (uint64_t(p[1]) << 24) | (uint64_t(p[2]) << 16) |
           (uint64_t(p[3]) << 8) | uint64_t(p[4]);
}

struct PsarcTocEntry {
    std::string name;
    uint32_t block_index = 0;   // first block index
    uint64_t uncompressed = 0;
    uint64_t file_offset = 0;   // absolute offset in archive of first block data
};

struct PsarcArchive {
    std::string path;
    bool zlib = true;
    uint32_t block_size = 65536;
    uint32_t toc_size = 0;
    std::vector<PsarcTocEntry> entries;
    std::vector<uint32_t> block_zsize; // compressed size per block (0 = full block stored)
    uint64_t data_start = 0;           // byte offset where block payloads begin
};

static std::vector<PsarcArchive> g_psarcs;

static bool psarc_load_toc(const char* path, PsarcArchive& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    uint8_t hdr[32];
    if (std::fread(hdr, 1, 32, f) != 32) { std::fclose(f); return false; }
    if (std::memcmp(hdr, "PSAR", 4) != 0) { std::fclose(f); return false; }

    out.path = path;
    out.zlib = (std::memcmp(hdr + 8, "zlib", 4) == 0) || (std::memcmp(hdr + 8, "slz2", 4) == 0);
    uint32_t toc_len = be32(hdr + 12);
    uint32_t toc_entsz = be32(hdr + 16);
    uint32_t toc_ents = be32(hdr + 20);
    out.block_size = be32(hdr + 24);
    if (out.block_size == 0) out.block_size = 65536;
    out.toc_size = toc_len;

    if (toc_len < 30 || toc_ents == 0 || toc_ents > 500000 || toc_entsz < 30) {
        std::fclose(f); return false;
    }

    std::vector<uint8_t> toc(toc_len);
    if (std::fread(toc.data(), 1, toc_len, f) != toc_len) { std::fclose(f); return false; }

    // Parse entries (30-byte): md5[16] + blockIndex(u32) + usize(u40) + offset(u40)
    out.entries.clear();
    out.entries.reserve(toc_ents);
    size_t entry_bytes = (size_t)toc_ents * (size_t)toc_entsz;
    if (entry_bytes > toc.size()) entry_bytes = toc.size();

    for (uint32_t i = 0; i < toc_ents; ++i) {
        size_t off = (size_t)i * toc_entsz;
        if (off + 30 > toc.size()) break;
        const uint8_t* e = toc.data() + off;
        PsarcTocEntry ent;
        ent.block_index = be32(e + 16);
        ent.uncompressed = be40(e + 20);
        ent.file_offset = be40(e + 25);
        out.entries.push_back(std::move(ent));
    }

    // Block size table follows TOC entries inside toc blob (or after)
    size_t blocks_off = entry_bytes;
    // Each block zsize is typically u16 BE when < block_size, but many archives use u16
    // Total blocks ≈ ceil(total_uncompressed / block_size) across files — use remaining TOC bytes
    size_t remain = toc.size() > blocks_off ? toc.size() - blocks_off : 0;
    out.block_zsize.clear();
    if (remain >= 2) {
        // Prefer u16 BE entries
        size_t n16 = remain / 2;
        out.block_zsize.reserve(n16);
        for (size_t i = 0; i < n16; ++i) {
            uint32_t z = (uint32_t(toc[blocks_off + i * 2]) << 8) | uint32_t(toc[blocks_off + i * 2 + 1]);
            out.block_zsize.push_back(z);
        }
    }

    // Data starts after 32-byte header + toc_len (aligned sometimes)
    out.data_start = 32ull + (uint64_t)toc_len;

    // Filename list is entry 0 — decompress its blocks into a string table
    if (!out.entries.empty()) {
        auto& names_ent = out.entries[0];
        names_ent.name = "(names)";
        uint64_t total_u = names_ent.uncompressed;
        if (total_u > 0 && total_u < 64ull * 1024 * 1024) {
            std::vector<uint8_t> names((size_t)total_u);
            uint64_t filled = 0;
            uint32_t bidx = names_ent.block_index;
            uint64_t file_pos = names_ent.file_offset ? names_ent.file_offset : out.data_start;
            // If file_offset looks like absolute, use it; else data_start + cumulative
            if (file_pos < out.data_start) file_pos = out.data_start + file_pos;

            while (filled < total_u && bidx < out.block_zsize.size()) {
                uint32_t zsz = out.block_zsize[bidx];
                if (zsz == 0) zsz = out.block_size;
                std::vector<uint8_t> zbuf(zsz);
                if (std::fseek(f, (long)file_pos, SEEK_SET) != 0) break;
                size_t got = std::fread(zbuf.data(), 1, zsz, f);
                if (got == 0) break;
                file_pos += got;

                size_t want = (size_t)std::min<uint64_t>(out.block_size, total_u - filled);
                size_t produced = 0;
                if (got == want || !out.zlib) {
                    size_t n = got < want ? got : want;
                    std::memcpy(names.data() + filled, zbuf.data(), n);
                    produced = n;
                } else {
                    zlib_inflate_block(zbuf.data(), got, names.data() + filled, want, &produced);
                }
                if (produced == 0) break;
                filled += produced;
                ++bidx;
            }

            // Split names on NUL
            size_t start = 0;
            size_t name_i = 0;
            for (size_t i = 0; i <= filled; ++i) {
                if (i == filled || names[i] == 0) {
                    if (i > start) {
                        std::string nm((char*)names.data() + start, i - start);
                        if (name_i < out.entries.size())
                            out.entries[name_i].name = nm;
                        ++name_i;
                    }
                    start = i + 1;
                }
            }
        }
    }

    // Fill empty names with index
    for (size_t i = 0; i < out.entries.size(); ++i) {
        if (out.entries[i].name.empty()) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "file_%zu", i);
            out.entries[i].name = buf;
        }
    }

    std::fclose(f);
    ps3rt_log("[psarc] XX-J %s entries=%zu block_size=%u blocks=%zu zlib=%d data_start=0x%llx\n",
              path, out.entries.size(), out.block_size, out.block_zsize.size(),
              out.zlib ? 1 : 0, (unsigned long long)out.data_start);
    return !out.entries.empty();
}

// ---------------- FD table + PSARC virtual file ----------------
struct HostFd {
    FILE* f = nullptr;
    bool used = false;
    char path[512] = {};
    bool is_psarc = false;
    int arc_index = -1;       // index into g_psarcs
    int entry_index = -1;
    uint64_t arc_off = 0;
    uint64_t arc_len = 0;     // uncompressed size
    uint64_t pos = 0;
    // Cached decompressed window for current logical block
    std::vector<uint8_t> cache;
    uint64_t cache_base = 0;  // logical offset of cache[0]
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

static bool psarc_fill_cache(HostFd* h, uint64_t logical_off) {
    if (!h || !h->is_psarc || h->arc_index < 0 || h->arc_index >= (int)g_psarcs.size())
        return false;
    if (h->entry_index < 0) return false;
    auto& arc = g_psarcs[h->arc_index];
    if (h->entry_index >= (int)arc.entries.size()) return false;
    auto& ent = arc.entries[h->entry_index];

    uint32_t bsz = arc.block_size ? arc.block_size : 65536;
    uint64_t block_logical = logical_off / bsz;
    uint64_t cache_base = block_logical * bsz;
    if (!h->cache.empty() && h->cache_base == cache_base && h->cache.size() > 0)
        return true;

    uint32_t bidx = ent.block_index + (uint32_t)block_logical;
    if (bidx >= arc.block_zsize.size()) {
        // No block table — treat as raw slice
        h->cache.assign(bsz, 0);
        h->cache_base = cache_base;
        if (!h->f) return false;
        uint64_t abs = (ent.file_offset ? ent.file_offset : arc.data_start) + cache_base;
        if (std::fseek(h->f, (long)abs, SEEK_SET) != 0) return false;
        size_t want = (size_t)std::min<uint64_t>(bsz, h->arc_len > cache_base ? h->arc_len - cache_base : 0);
        size_t n = std::fread(h->cache.data(), 1, want, h->f);
        h->cache.resize(n);
        return n > 0;
    }

    uint32_t zsz = arc.block_zsize[bidx];
    if (zsz == 0) zsz = bsz;

    // Absolute file offset of this block: start at entry offset, skip prior compressed sizes
    uint64_t abs = ent.file_offset ? ent.file_offset : arc.data_start;
    for (uint32_t i = ent.block_index; i < bidx; ++i) {
        uint32_t z = (i < arc.block_zsize.size()) ? arc.block_zsize[i] : bsz;
        if (z == 0) z = bsz;
        abs += z;
    }

    if (!h->f) return false;
    std::vector<uint8_t> zbuf(zsz);
    if (std::fseek(h->f, (long)abs, SEEK_SET) != 0) return false;
    size_t got = std::fread(zbuf.data(), 1, zsz, h->f);
    if (got == 0) return false;

    size_t want = (size_t)std::min<uint64_t>(bsz, h->arc_len > cache_base ? h->arc_len - cache_base : bsz);
    h->cache.assign(want, 0);
    size_t produced = 0;
    if (got == want || !arc.zlib || zsz == bsz) {
        size_t n = got < want ? got : want;
        std::memcpy(h->cache.data(), zbuf.data(), n);
        produced = n;
    } else {
        zlib_inflate_block(zbuf.data(), got, h->cache.data(), want, &produced);
        h->cache.resize(produced);
    }
    h->cache_base = cache_base;
    static int s_dec = 0;
    if (s_dec < 12) {
        ps3rt_log("[psarc] XX-J inflate block bidx=%u z=%u -> %zu logical=0x%llx\n",
                  bidx, zsz, produced, (unsigned long long)cache_base);
        ++s_dec;
    }
    return produced > 0;
}

static size_t psarc_read(HostFd* h, uint8_t* dst, size_t size) {
    if (!h || !dst || size == 0) return 0;
    size_t done = 0;
    while (done < size && h->pos < h->arc_len) {
        if (!psarc_fill_cache(h, h->pos)) break;
        uint64_t off_in = h->pos - h->cache_base;
        if (off_in >= h->cache.size()) break;
        size_t chunk = (size_t)std::min<uint64_t>(h->cache.size() - off_in, size - done);
        if (h->pos + chunk > h->arc_len) chunk = (size_t)(h->arc_len - h->pos);
        std::memcpy(dst + done, h->cache.data() + off_in, chunk);
        done += chunk;
        h->pos += chunk;
    }
    return done;
}

// ---------------- Scan ----------------
#ifdef _WIN32
static void scan_one_pattern(const char* pattern) {
    WIN32_FIND_DATAA fd{};
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    char dir[MAX_PATH];
    std::snprintf(dir, sizeof(dir), "%s", pattern);
    char* slash = strrchr(dir, '\\');
    if (slash) *slash = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        char full[MAX_PATH];
        std::snprintf(full, sizeof(full), "%s\\%s", dir, fd.cFileName);
        const char* name = fd.cFileName;
        size_t len = std::strlen(name);
        if (len >= 6 && ps3_stricmp(name + len - 6, ".psarc") == 0) {
            PsarcArchive a;
            if (psarc_load_toc(full, a))
                g_psarcs.push_back(std::move(a));
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

static void scan_psarc_recursive(const char* root, int depth) {
    if (depth > 3 || !root || !root[0]) return;
    char pattern[MAX_PATH];
    std::snprintf(pattern, sizeof(pattern), "%s\\*", root);
    WIN32_FIND_DATAA fd{};
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.cFileName[0] == '.' && (fd.cFileName[1] == 0 || (fd.cFileName[1] == '.' && fd.cFileName[2] == 0)))
            continue;
        char full[MAX_PATH];
        std::snprintf(full, sizeof(full), "%s\\%s", root, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_psarc_recursive(full, depth + 1);
        } else {
            size_t len = std::strlen(fd.cFileName);
            if (len >= 6 && ps3_stricmp(fd.cFileName + len - 6, ".psarc") == 0) {
                PsarcArchive a;
                if (psarc_load_toc(full, a))
                    g_psarcs.push_back(std::move(a));
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
#endif

static void psarc_scan_game_root() {
    ensure_game_root();
    g_psarcs.clear();
#ifdef _WIN32
    char pattern[MAX_PATH];
    const char* pats[] = {
        "%s\\*.psarc",
        "%s\\data\\*.psarc",
        "%s\\DATA\\*.psarc",
        "%s\\USRDIR\\*.psarc",
        "%s\\PS3_GAME\\USRDIR\\*.psarc",
        nullptr
    };
    for (int i = 0; pats[i]; ++i) {
        std::snprintf(pattern, sizeof(pattern), pats[i], g_game_root);
        scan_one_pattern(pattern);
    }
    scan_psarc_recursive(g_game_root, 0);
    // Parent folder (game root above USRDIR)
    char parent[MAX_PATH];
    std::snprintf(parent, sizeof(parent), "%s", g_game_root);
    char* slash = strrchr(parent, '\\');
    if (slash) {
        *slash = 0;
        std::snprintf(pattern, sizeof(pattern), "%s\\*.psarc", parent);
        scan_one_pattern(pattern);
        // one more up (title folder)
        slash = strrchr(parent, '\\');
        if (slash) {
            *slash = 0;
            std::snprintf(pattern, sizeof(pattern), "%s\\*.psarc", parent);
            scan_one_pattern(pattern);
        }
    }
#endif
    size_t total_ents = 0;
    for (auto& a : g_psarcs) total_ents += a.entries.size();
    ps3rt_log("[fs] XX-J asset scan done root=%s  psarcs=%zu  entries=%zu\n",
              g_game_root, g_psarcs.size(), total_ents);
}

// ---------------- cellFs ----------------
static void write_be64(uint64_t ea, uint64_t v) {
    if (!ea || ea + 8 > mem_size()) return;
    for (int i = 7; i >= 0; --i)
        mem()[ea + (7 - i)] = (uint8_t)((v >> (i * 8)) & 0xFF);
}
static void write_be32(uint64_t ea, uint32_t v) {
    if (!ea || ea + 4 > mem_size()) return;
    mem()[ea + 0] = (uint8_t)((v >> 24) & 0xFF);
    mem()[ea + 1] = (uint8_t)((v >> 16) & 0xFF);
    mem()[ea + 2] = (uint8_t)((v >> 8) & 0xFF);
    mem()[ea + 3] = (uint8_t)(v & 0xFF);
}

static int fs_open(PPUContext* c) {
    uint64_t path_ea = c->gpr[3];
    int flags = (int)(uint32_t)c->gpr[4];
    uint64_t fd_ea = c->gpr[5];
    char guest[512] = {};
    if (path_ea && path_ea < mem_size()) {
        size_t n = 0;
        while (n + 1 < sizeof(guest) && path_ea + n < mem_size()) {
            char ch = (char)mem()[path_ea + n];
            if (!ch) break;
            guest[n++] = ch;
        }
        guest[n] = 0;
    }
    char host[1024] = {};
    map_guest_path(guest, host, sizeof(host));

    int fd = -1;
    const char* mode = (flags & 2) ? "rb+" : "rb";
    FILE* f = std::fopen(host, mode);
    if (!f && (flags & 1)) f = std::fopen(host, "wb");
    if (f) {
        fd = fd_alloc(f, host);
    } else {
        // Virtual open inside PSARC by leaf name or relative path
        const char* leaf = guest;
        for (const char* q = guest; *q; ++q)
            if (*q == '/' || *q == '\\') leaf = q + 1;
        for (int ai = 0; ai < (int)g_psarcs.size(); ++ai) {
            auto& arc = g_psarcs[ai];
            for (int ei = 0; ei < (int)arc.entries.size(); ++ei) {
                auto& e = arc.entries[ei];
                if (e.name.empty() || e.name == "(names)") continue;
                // match full relative or leaf
                bool match = (ps3_stricmp(e.name.c_str(), guest) == 0) ||
                             (ps3_stricmp(e.name.c_str(), leaf) == 0);
                if (!match) {
                    // suffix match
                    size_t el = e.name.size(), gl = std::strlen(guest);
                    if (el >= gl && ps3_stricmp(e.name.c_str() + (el - gl), guest) == 0)
                        match = true;
                }
                if (!match) continue;
                FILE* af = std::fopen(arc.path.c_str(), "rb");
                if (!af) continue;
                fd = fd_alloc(af, arc.path.c_str());
                if (fd < 0) { std::fclose(af); continue; }
                g_fds[fd].is_psarc = true;
                g_fds[fd].arc_index = ai;
                g_fds[fd].entry_index = ei;
                g_fds[fd].arc_off = e.file_offset;
                g_fds[fd].arc_len = e.uncompressed ? e.uncompressed : 0x100000;
                g_fds[fd].pos = 0;
                ps3rt_log("[psarc] XX-J virtual open \"%s\" (usize=%llu) from %s\n",
                          e.name.c_str(), (unsigned long long)g_fds[fd].arc_len, arc.path.c_str());
                break;
            }
            if (fd >= 0) break;
        }
    }
    static int s_log = 0;
    if (s_log < 80) {
        ps3rt_log("[fs] open guest=\"%s\" host=\"%s\" %s\n", guest, host, fd >= 0 ? "OK" : "FAIL");
        ++s_log;
    }
    if (fd < 0) { c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
    if (fd_ea) write_be32(fd_ea, (uint32_t)fd);
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
    size_t n = 0;
    if (h->is_psarc)
        n = psarc_read(h, mem() + buf, (size_t)size);
    else
        n = std::fread(mem() + buf, 1, (size_t)size, h->f);
    if (nread_ea) write_be64(nread_ea, n);
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
    if (nwr_ea) write_be64(nwr_ea, n);
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
    h->cache.clear();
    c->gpr[3] = 0;
    return 0;
}

static int fs_lseek(PPUContext* c) {
    int fd = (int)(uint32_t)c->gpr[3];
    int64_t off = (int64_t)c->gpr[4];
    int whence = (int)(uint32_t)c->gpr[5];
    uint64_t pos_ea = c->gpr[6];
    HostFd* h = fd_get(fd);
    if (!h || !h->f) { c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
    if (h->is_psarc) {
        int64_t base = 0;
        if (whence == 1) base = (int64_t)h->pos;
        else if (whence == 2) base = (int64_t)h->arc_len;
        int64_t np = base + off;
        if (np < 0) np = 0;
        if ((uint64_t)np > h->arc_len) np = (int64_t)h->arc_len;
        h->pos = (uint64_t)np;
        if (pos_ea) write_be64(pos_ea, h->pos);
        c->gpr[3] = 0;
        return 0;
    }
    int w = SEEK_SET;
    if (whence == 1) w = SEEK_CUR;
    else if (whence == 2) w = SEEK_END;
    if (std::fseek(h->f, (long)off, w) != 0) {
        c->gpr[3] = (uint64_t)(int32_t)-1; return 0;
    }
    long cur = std::ftell(h->f);
    if (pos_ea && cur >= 0) write_be64(pos_ea, (uint64_t)cur);
    c->gpr[3] = 0;
    return 0;
}

static int fs_stat_common(PPUContext* c, bool by_fd) {
    // CellFsStat-like: fill size fields if pointer provided
    uint64_t st_ea = by_fd ? c->gpr[4] : c->gpr[4];
    uint64_t size = 0;
    if (by_fd) {
        HostFd* h = fd_get((int)(uint32_t)c->gpr[3]);
        if (!h) { c->gpr[3] = (uint64_t)(int32_t)-1; return 0; }
        size = h->is_psarc ? h->arc_len : 0;
        if (!h->is_psarc && h->f) {
            long cur = std::ftell(h->f);
            std::fseek(h->f, 0, SEEK_END);
            long end = std::ftell(h->f);
            if (cur >= 0) std::fseek(h->f, cur, SEEK_SET);
            if (end > 0) size = (uint64_t)end;
        }
    }
    // Minimal: st_size at a common offset (0x28 in many layouts) — write several candidates
    if (st_ea && st_ea + 0x40 <= mem_size()) {
        write_be64(st_ea + 0x28, size);
        write_be64(st_ea + 0x18, size);
        write_be32(st_ea + 0x0, 0x100); // mode-ish regular file
    }
    c->gpr[3] = 0;
    return 0;
}

PS3RT_API int ps3rt_fs_dispatch(PPUContext* c, uint32_t nid) {
    if (!c) return -1;
    std::lock_guard<std::mutex> lk(g_fs_mu);
    static bool scanned = false;
    if (!scanned) { scanned = true; psarc_scan_game_root(); }

    switch (nid) {
    case 0x718BF5F8: return fs_open(c);    // cellFsOpen
    case 0xB1840B49: // cellFsRead (alt)
    case 0x4D5B9EF2: return fs_read(c);    // cellFsRead
    case 0xECDCF2AB: return fs_write(c);   // cellFsWrite
    case 0x2CB51F0D: // alt Close
    case 0x2CB51F0F: return fs_close(c);   // cellFsClose
    case 0xA397D042: return fs_lseek(c);   // cellFsLseek
    case 0x7F13FC8C: return fs_stat_common(c, false); // cellFsStat
    case 0xEF3EFA34: return fs_stat_common(c, true);  // cellFsFstat
    case 0x3F61245C: // cellFsOpendir
    case 0x5C74903D: // cellFsReaddir
    case 0xFF42DCC3: // cellFsClosedir
    case 0x9B882495: // cellFsGetDirectoryEntries
        c->gpr[3] = 0; return 0;
    default:
        c->gpr[3] = 0;
        return 0;
    }
}

PS3RT_API void ps3rt_assets_boot_scan(void) {
    std::lock_guard<std::mutex> lk(g_fs_mu);
    zlib_load();
    psarc_scan_game_root();
}
