// ps3rt - guest memory + LV2 HLE + SPU + PRX/NID + pad stub
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <algorithm>
#include <vector>

static uint8_t* g_mem = nullptr;
static size_t g_mem_size = 0;
static const uint64_t kStackBase = 0x0F000000ull;
static const size_t   kStackSize = 0x100000;
static std::mutex g_lock;

static uint64_t g_alloc_ptr = 0x08000000ull;
static const uint64_t kAllocEnd = 0x0E000000ull;

static void wr_be64_guest(uint64_t addr, uint64_t val) {
    if (!g_mem || addr + 8 > g_mem_size) return;
    uint8_t b[8];
    for (int i = 7; i >= 0; --i) { b[i] = (uint8_t)(val & 0xFF); val >>= 8; }
    std::memcpy(g_mem + addr, b, 8);
}
static void wr_be32_guest(uint64_t addr, uint32_t val) {
    if (!g_mem || addr + 4 > g_mem_size) return;
    uint8_t b[4] = { (uint8_t)(val>>24), (uint8_t)(val>>16), (uint8_t)(val>>8), (uint8_t)val };
    std::memcpy(g_mem + addr, b, 4);
}
static uint32_t rd_be32_guest(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return ((uint32_t)g_mem[addr] << 24) | ((uint32_t)g_mem[addr+1] << 16) |
           ((uint32_t)g_mem[addr+2] << 8) | (uint32_t)g_mem[addr+3];
}

static uint64_t guest_alloc(uint64_t size, uint64_t align) {
    if (size == 0) size = 0x1000;
    if (align < 0x1000) align = 0x1000;
    if (align & (align - 1)) align = 0x10000;
    uint64_t a = (g_alloc_ptr + (align - 1)) & ~(align - 1);
    if (a + size > kAllocEnd || a + size > g_mem_size) {
        std::fprintf(stderr, "[lv2] guest_alloc FAIL size=0x%llx align=0x%llx\n",
            (unsigned long long)size, (unsigned long long)align);
        return 0;
    }
    std::memset(g_mem + a, 0, (size_t)size);
    g_alloc_ptr = a + size;
    std::fprintf(stderr, "[lv2] alloc VA=0x%llx size=0x%llx\n",
        (unsigned long long)a, (unsigned long long)size);
    return a;
}

PS3RT_API void ps3rt_touch(uint64_t, size_t) {}

static uint32_t rd_le32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
static uint64_t rd_le64(const uint8_t* p) { uint64_t v; std::memcpy(&v, p, 8); return v; }

static int load_guest_image(const char* path) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return -1;
    std::fseek(f, 0, SEEK_END);
    long fsz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (fsz < 12) { std::fclose(f); return -1; }
    std::vector<uint8_t> file((size_t)fsz);
    if (std::fread(file.data(), 1, (size_t)fsz, f) != (size_t)fsz) { std::fclose(f); return -1; }
    std::fclose(f);
    if (std::memcmp(file.data(), "PS3IMG1", 7) == 0) {
        uint32_t nseg = rd_le32(file.data() + 8);
        size_t hdr = 12ull + (size_t)nseg * 32ull;
        if (hdr > (size_t)fsz) return -1;
        int mapped = 0;
        for (uint32_t i = 0; i < nseg; ++i) {
            const uint8_t* e = file.data() + 12 + i * 32;
            uint64_t vaddr = rd_le64(e + 0);
            uint64_t filesz = rd_le64(e + 8);
            uint64_t memsz  = rd_le64(e + 16);
            uint64_t off    = rd_le64(e + 24);
            if (filesz == 0 && memsz == 0) continue;
            if (off + filesz > (size_t)fsz) continue;
            if (vaddr + memsz > g_mem_size) {
                if (vaddr >= g_mem_size) continue;
                memsz = g_mem_size - vaddr;
                if (filesz > memsz) filesz = memsz;
            }
            std::memset(g_mem + vaddr, 0, (size_t)memsz);
            if (filesz) std::memcpy(g_mem + vaddr, file.data() + (size_t)off, (size_t)filesz);
            std::fprintf(stderr, "[ps3rt] map seg%u VA=0x%llx filesz=0x%llx memsz=0x%llx\n",
                i, (unsigned long long)vaddr, (unsigned long long)filesz, (unsigned long long)memsz);
            ++mapped;
        }
        std::fprintf(stderr, "[ps3rt] PS3IMG1 loaded from %s (%d segments)\n", path, mapped);
        return mapped > 0 ? 0 : -1;
    }
    if ((size_t)fsz <= g_mem_size) {
        std::memcpy(g_mem, file.data(), (size_t)fsz);
        return 0;
    }
    return -1;
}

PS3RT_API int ps3rt_init(const char* image_path) {
    std::lock_guard<std::mutex> lk(g_lock);
    if (g_mem) return 0;
    g_mem_size = 512ull * 1024 * 1024;
    g_mem = (uint8_t*)std::calloc(1, g_mem_size);
    if (!g_mem) return -1;
    g_alloc_ptr = 0x08000000ull;
    const char* candidates[] = { image_path, "guest_image.bin", "output/guest_image.bin" };
    bool ok = false;
    for (const char* p : candidates) {
        if (!p) continue;
        if (load_guest_image(p) == 0) { ok = true; break; }
    }
    if (!ok) std::fprintf(stderr, "[ps3rt] warning: no guest_image.bin mapped\n");
    return 0;
}

PS3RT_API uint8_t* ps3rt_memory(void) { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize - 0x100; }
PS3RT_API void ps3rt_shutdown(void) {
    std::lock_guard<std::mutex> lk(g_lock);
    std::free(g_mem); g_mem = nullptr; g_mem_size = 0;
}

static constexpr int kMaxSpu = 6;
static SPUContext g_spu[kMaxSpu];
static bool g_spu_used[kMaxSpu] = {};

PS3RT_API int ps3rt_spu_supported(void) { return 1; }
PS3RT_API int ps3rt_spu_create(int* out_id) {
    for (int i = 0; i < kMaxSpu; ++i) if (!g_spu_used[i]) {
        g_spu_used[i] = true; std::memset(&g_spu[i], 0, sizeof(SPUContext));
        if (out_id) *out_id = i; return 0;
    }
    return -1;
}
PS3RT_API int ps3rt_spu_destroy(int id) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu_used[id] = false; return 0;
}
PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !img) return -1;
    std::memcpy(g_spu[id].ls, img, std::min(len, sizeof(g_spu[id].ls))); return 0;
}
PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].pc = entry & ~3u; g_spu[id].running = true; return 0;
}
PS3RT_API int ps3rt_spu_stop(int id) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].running = false; return 0;
}
PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    if (g_spu[id].mailbox_in_count >= 4) return -1;
    g_spu[id].mailbox_in[g_spu[id].mailbox_in_count++] = val; return 0;
}
PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !out) return -1;
    if (g_spu[id].mailbox_out_count <= 0) return -1;
    *out = g_spu[id].mailbox_out[0];
    for (int i = 1; i < g_spu[id].mailbox_out_count; ++i)
        g_spu[id].mailbox_out[i-1] = g_spu[id].mailbox_out[i];
    --g_spu[id].mailbox_out_count; return 0;
}
PS3RT_API int ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !g_mem) return -1;
    lsa &= 0x3FFFF;
    if (lsa + size > sizeof(g_spu[id].ls) || ea + size > g_mem_size) return -1;
    if ((cmd & 0x1F) == 0x40 || (cmd & 0x1F) == 0x41)
        std::memcpy(g_spu[id].ls + lsa, g_mem + ea, size);
    else
        std::memcpy(g_mem + ea, g_spu[id].ls + lsa, size);
    return 0;
}

static uint16_t g_pad_buttons = 0;
static int16_t  g_pad_lx = 0x80, g_pad_ly = 0x80, g_pad_rx = 0x80, g_pad_ry = 0x80;

PS3RT_API void ps3rt_pad_set(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
    g_pad_buttons = buttons;
    g_pad_lx = lx; g_pad_ly = ly; g_pad_rx = rx; g_pad_ry = ry;
}

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    if (!c) return;
    uint64_t num = c->gpr[11];
    uint64_t r3 = c->gpr[3], r4 = c->gpr[4], r5 = c->gpr[5], r6 = c->gpr[6];
    static int sys_log = 0;
    if (sys_log < 128) {
        std::fprintf(stderr, "[lv2] sc %llu r3=%llx r4=%llx r5=%llx r6=%llx\n",
            (unsigned long long)num, (unsigned long long)r3, (unsigned long long)r4,
            (unsigned long long)r5, (unsigned long long)r6);
        ++sys_log;
    }

    auto write_u32 = [&](uint64_t a, uint32_t v) {
        if (a && a + 4 <= g_mem_size) wr_be32_guest(a, v);
    };
    auto write_u64 = [&](uint64_t a, uint64_t v) {
        if (a && a + 8 <= g_mem_size) wr_be64_guest(a, v);
    };
    auto new_id = [](uint32_t& seq) -> uint32_t { return ++seq; };

    switch (num) {
    case 1: c->halted = true; c->gpr[3] = 0; break;
    case 2: c->gpr[3] = 1; break;
    case 3:
        if ((r3 == 1 || r3 == 2) && g_mem && r4 + r5 <= g_mem_size) {
            std::fwrite(g_mem + r4, 1, (size_t)r5, stderr);
            c->gpr[3] = r5;
        } else c->gpr[3] = 0;
        break;
    case 4: c->gpr[3] = 0; break;
    case 14: c->halted = true; c->gpr[3] = 0; break;
    case 25: write_u32(r3, 0x00440000); c->gpr[3] = 0; break;

    case 18: {
        uint64_t size = r3, out = r5;
        uint64_t va = guest_alloc(size, 0x10000);
        if (va) write_u64(out, va);
        c->gpr[3] = va ? 0 : 0x80010004ull;
        break;
    }
    case 19: c->gpr[3] = 0; break;
    case 20: write_u32(r4, 0x400); c->gpr[3] = 0; break;
    case 22: write_u64(r3, 480ull * 1024 * 1024); c->gpr[3] = 0; break;
    case 23: case 24: c->gpr[3] = 0; break;

    case 30: write_u64(r3, 1); write_u64(r4, 0); c->gpr[3] = 0; break;
    case 31: c->gpr[3] = 79800000ull; break;
    case 70: case 73: c->gpr[3] = 0; break;

    case 41: { static uint32_t tid = 0x2000; write_u64(r3, new_id(tid)); c->gpr[3] = 0; break; }
    case 43: case 44: case 48: case 49: case 50: case 51: c->gpr[3] = 0; break;
    case 52: { static uint32_t self = 0x1000; write_u64(r3, self); c->gpr[3] = 0; break; }
    case 53: case 54: case 55: case 56: case 57: case 58: c->gpr[3] = 0; break;

    case 90: { static uint32_t mid = 0x3000; write_u32(r3, new_id(mid)); c->gpr[3] = 0; break; }
    case 91: case 92: case 93: case 94: case 95: c->gpr[3] = 0; break;
    case 96: { static uint32_t lwid = 0x4000; write_u32(r3, new_id(lwid)); c->gpr[3] = 0; break; }
    case 97: case 98: case 99: case 100: case 101: c->gpr[3] = 0; break;
    case 105: { static uint32_t cid = 0x5000; write_u32(r3, new_id(cid)); c->gpr[3] = 0; break; }
    case 106: case 107: case 108: case 109: case 110: c->gpr[3] = 0; break;
    case 114: case 115: case 116: case 117: case 118: case 119: c->gpr[3] = 0; break;
    case 120: { static uint32_t sid = 0x6000; write_u32(r3, new_id(sid)); c->gpr[3] = 0; break; }
    case 121: case 122: case 123: case 124: case 125: c->gpr[3] = 0; break;
    case 128: { static uint32_t eid = 0x7000; write_u32(r3, new_id(eid)); c->gpr[3] = 0; break; }
    case 129: case 130: case 131: case 132: c->gpr[3] = 0; break;
    case 133: case 134: c->gpr[3] = 0x8001000Aull; break;
    case 135: case 136: case 137: case 138: case 139: case 140: c->gpr[3] = 0; break;
    case 143: { static uint32_t fid = 0x8000; write_u32(r3, new_id(fid)); c->gpr[3] = 0; break; }
    case 141: case 142: case 144: case 145: case 146: case 147: case 148: case 149: c->gpr[3] = 0; break;

    case 160: case 161: case 162: case 163: case 164: case 165:
    case 166: case 167: case 168: case 169: case 170: case 171:
    case 172: case 173: case 174: case 175: case 176: case 177:
        c->gpr[3] = 0; break;
    case 348: write_u32(r4, 1); c->gpr[3] = 0; break;
    case 349: case 350: case 351: c->gpr[3] = 0; break;

    case 352: {
        uint64_t size = r3, flags = r4, align = r5 ? r5 : 0x10000, out = r6;
        if (!out && flags >= 0x10000 && flags + 8 <= g_mem_size) out = flags;
        if (size == 0 || size >= 0x08000000ull) {
            std::fprintf(stderr, "[lv2] mmapper size 0x%llx -> 1MiB (out=0x%llx)\n",
                (unsigned long long)size, (unsigned long long)out);
            size = 0x100000;
        } else if (size > (kAllocEnd - 0x08000000ull))
            size = 0x100000;
        if (align < 0x1000) align = 0x10000;
        uint64_t va = guest_alloc(size, align);
        if (va && out && out + 8 <= g_mem_size) {
            wr_be64_guest(out, va);
            std::fprintf(stderr, "[lv2] mmapper wrote *0x%llx = 0x%llx\n",
                (unsigned long long)out, (unsigned long long)va);
        } else if (va && out && out + 4 <= g_mem_size) {
            wr_be32_guest(out, (uint32_t)va);
        }
        c->gpr[3] = va ? 0 : 0x80010004ull;
        break;
    }
    case 353: case 354: case 355: case 356: case 357: case 358: case 359:
        c->gpr[3] = 0; break;

    case 400: case 401: case 402: c->gpr[3] = 0; break;
    case 403: { static uint32_t mid = 0x9000; write_u32(r4, new_id(mid)); c->gpr[3] = 0; break; }
    case 404: case 405: case 406: case 407: case 408: case 409:
    case 410: case 411: case 412: case 413: case 414: case 415:
        c->gpr[3] = 0; break;

    case 801: { static uint32_t fd = 3; write_u32(r4, new_id(fd)); c->gpr[3] = 0; break; }
    case 802: c->gpr[3] = 0; break;
    case 803: c->gpr[3] = r5; break;
    case 804: case 805: case 806: case 807: case 808: case 809: case 810:
    case 811: case 812: case 813: case 814: case 815: case 816: case 817:
    case 818: case 819: case 820: case 821: case 822: case 823: case 824:
        c->gpr[3] = 0; break;

    case 480: case 481: case 482: case 483: case 484: case 485:
    case 486: case 487: case 488: case 489: case 490: case 491:
    case 492: case 493: case 494: case 495: case 496: case 497:
        c->gpr[3] = 0; break;
    case 867:
        if (r3 && r3 + 16 <= g_mem_size)
            for (int i = 0; i < 16; ++i) g_mem[r3 + i] = (uint8_t)(0x10 + i);
        c->gpr[3] = 0; break;
    case 871: case 872: case 873: c->gpr[3] = 0; break;
    case 668: case 669: case 670: case 671: case 672: case 673:
        c->gpr[3] = 0; break;

    default:
        c->gpr[3] = 0;
        break;
    }
}

static int g_prx_log = 0;
static uint64_t g_gcm_fifo = 0;
static uint32_t g_gcm_fifo_size = 0x100000;
static int g_gcm_inited = 0;

static uint32_t rd_be32_raw(uint64_t addr) {
    if (!g_mem || addr + 4 > g_mem_size) return 0;
    return ((uint32_t)g_mem[addr] << 24) | ((uint32_t)g_mem[addr+1] << 16) |
           ((uint32_t)g_mem[addr+2] << 8) | (uint32_t)g_mem[addr+3];
}

static uint32_t recover_nid_from_callsite(uint64_t lr) {
    if (!lr || lr < 16) return 0;
    for (int back = 1; back <= 8; ++back) {
        uint64_t a = lr - (uint64_t)back * 4;
        uint32_t op = rd_be32_raw(a);
        if ((op >> 26) == 24) {
            uint32_t lo = op & 0xFFFF;
            for (int b2 = back + 1; b2 <= back + 6; ++b2) {
                uint64_t a2 = lr - (uint64_t)b2 * 4;
                uint32_t op2 = rd_be32_raw(a2);
                if ((op2 >> 26) == 15 && ((op2 >> 16) & 0x1F) == 0) {
                    uint32_t rd1 = (op >> 21) & 0x1F;
                    uint32_t rd2 = (op2 >> 21) & 0x1F;
                    if (rd1 == rd2) {
                        uint32_t hi = (uint32_t)(int16_t)(op2 & 0xFFFF);
                        uint32_t nid = (hi << 16) | lo;
                        if (nid && nid != 0x39800000u) return nid;
                    }
                }
            }
        }
    }
    return 0;
}

static int prx_dispatch(PPUContext* c, int kind, const char* name) {
    uint32_t cands[5] = {
        (uint32_t)c->gpr[11], (uint32_t)c->gpr[0], (uint32_t)c->gpr[12],
        (uint32_t)c->gpr[3], recover_nid_from_callsite(c->lr)
    };
    const NidEntry* e = nullptr;
    uint32_t maybe_nid = 0;
    for (int i = 0; i < 5; ++i) {
        uint32_t n = cands[i];
        if (!n || n == 0x39800000u || n < 0x100) continue;
        e = nid_lookup(n);
        if (e) { maybe_nid = n; break; }
        if (!maybe_nid) maybe_nid = n;
    }
    if (e) {
        if (g_prx_log < 64) {
            std::fprintf(stderr, "[prx] %s::%s nid=0x%08X\n", e->module, e->name, e->nid);
            ++g_prx_log;
        }
        kind = e->kind;
        if (e->nid == 0x15BAE46B) {
            uint64_t fifo = c->gpr[3];
            uint32_t size = (uint32_t)c->gpr[4];
            if (!size) size = 0x100000;
            g_gcm_fifo = fifo; g_gcm_fifo_size = size; g_gcm_inited = 1;
            ps3rt_rsx_init(fifo, size);
            c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0xD81D0D2D) { ps3rt_rsx_flip((uint32_t)c->gpr[3]); c->gpr[3] = 0; return 0; }
        if (e->nid == 0xA547ADDE || e->nid == 0xB2E761D4 || e->nid == 0xF80196C0 ||
            e->nid == 0xA53D12AE || e->nid == 0x98396A7C) { c->gpr[3] = 0; return 0; }
        if (e->nid == 0x55A14C05) {
            static uint64_t ctrl = 0;
            if (!ctrl) ctrl = guest_alloc(0x1000, 0x1000);
            c->gpr[3] = ctrl; return 0;
        }
        if (e->nid == 0x5F909B17) {
            static uint64_t labels = 0;
            if (!labels) labels = guest_alloc(0x2000, 0x1000);
            c->gpr[3] = labels + ((c->gpr[3] & 0xFF) * 0x10); return 0;
        }
        if (e->nid == 0x4524FE95) {
            uint64_t out = c->gpr[3];
            if (out && out + 32 <= g_mem_size) {
                wr_be32_guest(out + 0, 1280); wr_be32_guest(out + 4, 720); wr_be32_guest(out + 8, 0);
            }
            c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x1C936E6D || e->nid == 0x0D5F2C14 || e->nid == 0x3F797DFF) { c->gpr[3] = 0; return 0; }
        if (e->nid == 0xA703A51D) {
            uint64_t out = c->gpr[3];
            if (out && out + 16 <= g_mem_size) {
                wr_be32_guest(out + 0, 1); wr_be32_guest(out + 4, 1); wr_be32_guest(out + 8, 1);
            }
            c->gpr[3] = 0; return 0;
        }
        if (e->nid == 0x3AAAD464) {
            uint64_t out = c->gpr[4];
            if (out && out + 24 <= g_mem_size) {
                wr_be32_guest(out + 0, 24);
                g_mem[out + 4] = (uint8_t)(g_pad_buttons >> 8);
                g_mem[out + 5] = (uint8_t)(g_pad_buttons & 0xFF);
                g_mem[out + 6] = (uint8_t)g_pad_lx;
                g_mem[out + 7] = (uint8_t)g_pad_ly;
                g_mem[out + 8] = (uint8_t)g_pad_rx;
                g_mem[out + 9] = (uint8_t)g_pad_ry;
            }
            c->gpr[3] = 0; return 0;
        }
        if (kind >= 2 && kind <= 10) { c->gpr[3] = 0; return 0; }
    } else if (g_prx_log < 40) {
        std::fprintf(stderr, "[prx] stub kind=%d %s pc=0x%llx nid=0x%08X lr=0x%llx r11=0x%llx\n",
            kind, name ? name : "?", (unsigned long long)c->pc, maybe_nid,
            (unsigned long long)c->lr, (unsigned long long)c->gpr[11]);
        ++g_prx_log;
    }
    (void)kind; (void)name;
    if (kind == 1 && !g_gcm_inited) {
        g_gcm_inited = 1;
        ps3rt_rsx_init(0, 0x100000);
    }
    c->gpr[3] = 0;
    return 0;
}

PS3RT_API int ps3rt_prx_import_stub(PPUContext* c, uint64_t pc) {
    if (!c) return -1;
    if (pc == 0x39800000ull || (pc & 0xFFFF0000ull) == 0x39800000ull)
        return prx_dispatch(c, 1, "cellGcmSys");
    return prx_dispatch(c, prx_module_kind("unknown"), "import");
}

PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int n = 0;
    if (n < 8) {
        std::fprintf(stderr, "[ps3] unimplemented op=0x%08X pc=0x%llx\n", opcode, (unsigned long long)pc);
        ++n;
    }
    if (c) c->halted = true;
}
