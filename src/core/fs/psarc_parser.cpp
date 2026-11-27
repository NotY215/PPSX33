// WORK: Host-side PSARC TOC reader for any inner entry (.wad/.ts/.bin/…).
#include "psarc_parser.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <string>

namespace ppsx33::fs {
namespace {

struct Entry {
    std::string name;
    uint32_t block_index = 0;
    uint64_t uncompressed = 0;
    uint64_t file_offset = 0;
};

struct Arch {
    std::string path;
    uint32_t block_size = 65536;
    std::vector<Entry> entries;
    std::vector<uint32_t> block_zsize;
    uint64_t data_start = 0;
};

static Arch g_arc;
static std::vector<std::string> g_names;
static bool g_open = false;

static uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
static uint64_t be40(const uint8_t* p) {
    return (uint64_t(p[0]) << 32) | (uint64_t(p[1]) << 24) | (uint64_t(p[2]) << 16) |
           (uint64_t(p[3]) << 8) | uint64_t(p[4]);
}
static const char* leaf_of(const char* p) {
    const char* l = p;
    for (const char* q = p; *q; ++q)
        if (*q == '/' || *q == '\\') l = q + 1;
    return l;
}
static int icmp(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a++, cb = *b++;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

} // namespace

void psarc_close() {
    g_arc = Arch{};
    g_names.clear();
    g_open = false;
}

bool psarc_open(const std::string& path) {
    psarc_close();
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    uint8_t hdr[32];
    if (std::fread(hdr, 1, 32, f) != 32) { std::fclose(f); return false; }
    if (std::memcmp(hdr, "PSAR", 4) != 0) { std::fclose(f); return false; }

    g_arc.path = path;
    uint32_t toc_len = be32(hdr + 12);
    uint32_t toc_entsz = be32(hdr + 16);
    uint32_t toc_ents = be32(hdr + 20);
    g_arc.block_size = be32(hdr + 24);
    if (g_arc.block_size == 0) g_arc.block_size = 65536;
    if (toc_len < 30 || toc_ents == 0 || toc_ents > 500000 || toc_entsz < 30) {
        std::fclose(f); return false;
    }
    std::vector<uint8_t> toc(toc_len);
    if (std::fread(toc.data(), 1, toc_len, f) != toc_len) { std::fclose(f); return false; }

    g_arc.entries.assign(toc_ents, Entry{});
    size_t off = 0;
    for (uint32_t i = 0; i < toc_ents && off + 30 <= toc.size(); ++i) {
        const uint8_t* e = toc.data() + off;
        g_arc.entries[i].block_index = be32(e + 16);
        g_arc.entries[i].uncompressed = be40(e + 20);
        g_arc.entries[i].file_offset = be40(e + 25);
        off += toc_entsz;
    }
    g_arc.block_zsize.clear();
    while (off + 2 <= toc.size()) {
        g_arc.block_zsize.push_back((uint32_t(toc[off]) << 8) | toc[off + 1]);
        off += 2;
    }
    g_arc.data_start = 32ull + (uint64_t)toc_len;

    if (!g_arc.entries.empty() && g_arc.entries[0].uncompressed > 0 &&
        g_arc.entries[0].uncompressed < 64ull * 1024 * 1024) {
        auto& ne = g_arc.entries[0];
        std::vector<uint8_t> names((size_t)ne.uncompressed);
        uint64_t pos = ne.file_offset ? ne.file_offset : g_arc.data_start;
        size_t filled = 0;
        uint32_t bidx = ne.block_index;
        while (filled < names.size()) {
            uint32_t zsz = g_arc.block_size;
            if (bidx < g_arc.block_zsize.size() && g_arc.block_zsize[bidx] != 0)
                zsz = g_arc.block_zsize[bidx];
            std::vector<uint8_t> zbuf(zsz);
            if (std::fseek(f, (long)pos, SEEK_SET) != 0) break;
            size_t got = std::fread(zbuf.data(), 1, zsz, f);
            if (got == 0) break;
            pos += got;
            size_t want = std::min((size_t)g_arc.block_size, names.size() - filled);
            size_t n = got < want ? got : want;
            std::memcpy(names.data() + filled, zbuf.data(), n);
            filled += n;
            ++bidx;
        }
        size_t start = 0, name_i = 0;
        for (size_t i = 0; i <= filled; ++i) {
            if (i == filled || names[i] == 0) {
                if (i > start) {
                    std::string nm((char*)names.data() + start, i - start);
                    if (name_i < g_arc.entries.size())
                        g_arc.entries[name_i].name = nm;
                    ++name_i;
                }
                start = i + 1;
            }
        }
    }
    for (size_t i = 0; i < g_arc.entries.size(); ++i) {
        if (g_arc.entries[i].name.empty()) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "file_%zu", i);
            g_arc.entries[i].name = buf;
        }
    }
    g_names.clear();
    for (auto& e : g_arc.entries)
        if (!e.name.empty() && e.name != "(names)")
            g_names.push_back(e.name);
    std::fclose(f);
    g_open = true;
    return true;
}

bool psarc_read(const std::string& entry, std::vector<uint8_t>& out) {
    out.clear();
    if (!g_open) return false;
    const Entry* hit = nullptr;
    for (auto& e : g_arc.entries) {
        if (e.name.empty()) continue;
        if (icmp(e.name.c_str(), entry.c_str()) == 0 ||
            icmp(leaf_of(e.name.c_str()), entry.c_str()) == 0 ||
            icmp(leaf_of(e.name.c_str()), leaf_of(entry.c_str())) == 0) {
            hit = &e; break;
        }
        size_t el = e.name.size(), gl = entry.size();
        if (el >= gl && icmp(e.name.c_str() + (el - gl), entry.c_str()) == 0) {
            hit = &e; break;
        }
    }
    if (!hit || hit->uncompressed == 0 || hit->uncompressed > 256ull * 1024 * 1024)
        return false;
    FILE* f = std::fopen(g_arc.path.c_str(), "rb");
    if (!f) return false;
    out.resize((size_t)hit->uncompressed);
    uint64_t pos = hit->file_offset ? hit->file_offset : g_arc.data_start;
    size_t filled = 0;
    uint32_t bidx = hit->block_index;
    while (filled < out.size()) {
        uint32_t zsz = g_arc.block_size;
        if (bidx < g_arc.block_zsize.size() && g_arc.block_zsize[bidx] != 0)
            zsz = g_arc.block_zsize[bidx];
        std::vector<uint8_t> zbuf(zsz);
        if (std::fseek(f, (long)pos, SEEK_SET) != 0) break;
        size_t got = std::fread(zbuf.data(), 1, zsz, f);
        if (got == 0) break;
        pos += got;
        size_t want = std::min((size_t)g_arc.block_size, out.size() - filled);
        size_t n = got < want ? got : want;
        std::memcpy(out.data() + filled, zbuf.data(), n);
        filled += n;
        ++bidx;
    }
    std::fclose(f);
    if (filled < out.size()) out.resize(filled);
    return !out.empty();
}

const std::vector<std::string>& psarc_list() { return g_names; }

}
