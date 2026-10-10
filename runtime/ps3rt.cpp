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
    if (sys_log < 96) {
        std::fprintf(stderr, "[lv2] sc %llu r3=%llx r4=%llx r5=%llx r6=%llx\n",
            (unsigned long long)num, (unsigned long long)r3, (unsigned long long)r4,
            (unsigned long long)r5, (unsigned long long)r6);
        ++sys_log;
    }
    switch (num) {
    case 1: c->halted = true; break;
    case 3:
        if ((r3 == 1 || r3 == 2) && g_mem) {
            if (r4 + r5 <= g_mem_size) std::fwrite(g_mem + r4, 1, (size_t)r5, stderr);
            c->gpr[3] = r5;
        } else c->gpr[3] = 0;
        break;
    case 4: c->gpr[3] = 0; break;
    case 14:
        c->halted = true; c->gpr[3] = 0; break;
    case 18: {
        uint64_t size = r3, out = r5;
        uint64_t va = guest_alloc(size, 0x10000);
        if (va && out && out + 8 <= g_mem_size) wr_be64_guest(out, va);
        c->gpr[3] = va ? 0 : 0x80010004ull;
        break;
    }
    case 19: c->gpr[3] = 0; break;
    case 20: case 22: case 25:
        if (r3 && r3 + 8 <= g_mem_size) wr_be64_guest(r3, 480ull * 1024 * 1024);
        c->gpr[3] = 0; break;
    case 30: case 31: c->gpr[3] = 0; break;
    case 41: case 43: case 44: case 48: case 52: case 53:
        if ((num == 41 || num == 52) && r4 && r4 + 8 <= g_mem_size) {
            static uint32_t tid = 0x2000;
            wr_be32_guest(r4, ++tid);
        }
        c->gpr[3] = 0; break;
    case 70: case 73: c->gpr[3] = 0; break;
    case 90: case 91: case 93: case 94:
        if (num == 90 && r4 && r4 + 4 <= g_mem_size) {
            static uint32_t mid = 0x3000;
            wr_be32_guest(r4, ++mid);
        }
        c->gpr[3] = 0; break;
    case 96: case 97: case 98: case 99: case 100:
        if (num == 96 && r4 && r4 + 4 <= g_mem_size) {
            static uint32_t lwid = 0x4000;
            wr_be32_guest(r4, ++lwid);
        }
        c->gpr[3] = 0; break;
    case 105: case 106: case 107: case 108: case 109:
        if (num == 105 && r4 && r4 + 4 <= g_mem_size) {
            static uint32_t cid = 0x5000;
            wr_be32_guest(r4, ++cid);
        }
        c->gpr[3] = 0; break;
    case 114: case 115: case 116: case 117: case 118: c->gpr[3] = 0; break;
    case 120: case 121: case 122: case 123: case 124:
        if (num == 120 && r4 && r4 + 4 <= g_mem_size) {
            static uint32_t sid = 0x6000;
            wr_be32_guest(r4, ++sid);
        }
        c->gpr[3] = 0; break;
    case 128: case 129: case 130: case 131: case 132:
        if (num == 128 && r4 && r4 + 4 <= g_mem_size) {
            static uint32_t eid = 0x7000;
            wr_be32_guest(r4, ++eid);
        }
        c->gpr[3] = 0; break;
    case 133: case 134: c->gpr[3] = 0x8001000Aull; break;
    case 135: case 136: case 137: case 138: c->gpr[3] = 0; break;
    case 141: case 142: case 143: case 144: case 145:
        if ((num == 141 || num == 143) && r4 && r4 + 4 <= g_mem_size) {
            static uint32_t fid = 0x8000;
            wr_be32_guest(r4, ++fid);
        }
        c->gpr[3] = 0; break;
    case 348: case 349:
        if (r4 && r4 + 4 <= g_mem_size) wr_be32_guest(r4, 1);
        c->gpr[3] = 0; break;
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
            std::fprintf(stderr, "[lv2] mmapper wrote32 *0x%llx = 0x%x\n",
                (unsigned long long)out, (unsigned)va);
        } else {
            std::fprintf(stderr, "[lv2] mmapper NO out write va=0x%llx out=0x%llx\n",
                (unsigned long long)va, (unsigned long long)out);
        }
        c->gpr[3] = va ? 0 : 0x80010004ull;
        break;
    }
    case 353: case 354: case 355: case 356: case 357: c->gpr[3] = 0; break;
    case 403:
        if (r4 && r4 + 4 <= g_mem_size) {
            static uint32_t mid = 0x9000;
            wr_be32_guest(r4, ++mid);
        }
        c->gpr[3] = 0; break;
    case 480: case 481: case 482: case 483: case 484: case 485:
    case 486: case 487: case 488: case 494: c->gpr[3] = 0; break;
    case 801: case 802: case 803: case 804: case 808: case 809:
    case 811: case 812: case 814: c->gpr[3] = 0; break;
    default: c->gpr[3] = 0; break;
    }
}

static int g_prx_log = 0;
static uint64_t g_gcm_fifo = 0;
static uint32_t g_gcm_fifo_size = 0x100000;
static int g_gcm_inited = 0;

static int prx_dispatch(PPUContext* c, int kind, const char* name) {
    uint32_t maybe_nid = (uint32_t)c->gpr[11];
    if (!nid_lookup(maybe_nid))
        maybe_nid = (uint32_t)c->gpr[0];
    const NidEntry* e = nid_lookup(maybe_nid);
    if (e) {
        if (g_prx_log < 64) {
            std::fprintf(stderr, "[prx] %s::%s nid=0x%08X\n", e->module, e->name, e->nid);
            ++g_prx_log;
        }
        kind = e->kind;
        name = e->name;
        if (e->nid == 0x15BAE46B) {
            uint64_t fifo = c->gpr[3];
            uint32_t size = (uint32_t)c->gpr[4];
            if (!size) size = 0x100000;
            g_gcm_fifo = fifo;
            g_gcm_fifo_size = size;
            g_gcm_inited = 1;
            ps3rt_rsx_init(fifo, size);
            c->gpr[3] = 0;
            return 0;
        }
        if (e->nid == 0xD81D0D2D) {
            ps3rt_rsx_flip((uint32_t)c->gpr[3]);
            c->gpr[3] = 0;
            return 0;
        }
        if (e->nid == 0xA547ADDE || e->nid == 0xB2E761D4 || e->nid == 0xF80196C0 ||
            e->nid == 0xA53D12AE || e->nid == 0x98396A7C) {
            c->gpr[3] = 0;
            return 0;
        }
        if (e->nid == 0x55A14C05) {
            static uint64_t ctrl = 0;
            if (!ctrl) ctrl = guest_alloc(0x1000, 0x1000);
            c->gpr[3] = ctrl;
            return 0;
        }
        if (e->nid == 0x5F909B17) {
            static uint64_t labels = 0;
            if (!labels) labels = guest_alloc(0x2000, 0x1000);
            c->gpr[3] = labels + ((c->gpr[3] & 0xFF) * 0x10);
            return 0;
        }
        if (e->nid == 0x4524FE95) {
            uint64_t out = c->gpr[3];
            if (out && out + 32 <= g_mem_size) {
                wr_be32_guest(out + 0, 1280);
                wr_be32_guest(out + 4, 720);
                wr_be32_guest(out + 8, 0);
            }
            c->gpr[3] = 0;
            return 0;
        }
        if (e->nid == 0x1C936E6D || e->nid == 0x0D5F2C14 || e->nid == 0x3F797DFF) {
            c->gpr[3] = 0;
            return 0;
        }
        if (e->nid == 0xA703A51D) {
            uint64_t out = c->gpr[3];
            if (out && out + 16 <= g_mem_size) {
                wr_be32_guest(out + 0, 1);
                wr_be32_guest(out + 4, 1);
                wr_be32_guest(out + 8, 1);
            }
            c->gpr[3] = 0;
            return 0;
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
            c->gpr[3] = 0;
            return 0;
        }
        if (kind >= 2 && kind <= 10) {
            c->gpr[3] = 0;
            return 0;
        }
    } else if (g_prx_log < 32) {
        std::fprintf(stderr, "[prx] stub kind=%d %s pc=0x%llx nid=0x%08X\n",
            kind, name ? name : "?", (unsigned long long)c->pc, maybe_nid);
        ++g_prx_log;
    }
    (void)kind; (void)name;
    if (kind == 1) { c->gpr[3] = 0; return 0; }
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
