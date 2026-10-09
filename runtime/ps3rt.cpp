// ps3rt - runtime library (ps3rt.dll)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif

namespace {
constexpr uint64_t kGuestSize = 1ull << 32; // 4 GB low address space
constexpr uint64_t kStackBase = 0xD0000000ull;
constexpr uint64_t kStackSize = 2ull << 20; // 2 MB stack
constexpr uint64_t kHeapBase  = 0x30000000ull;
constexpr uint64_t kHeapSize  = 0x08000000ull; // 128 MB heap pool
uint8_t* g_mem = nullptr;
uint64_t g_heap_next = kHeapBase;

std::mutex g_spu_mu;
constexpr int kMaxSpu = 8;
SPUContext g_spu[kMaxSpu];
bool g_spu_used[kMaxSpu] = {};

bool map_region(uint64_t addr, uint64_t size) {
    if (!g_mem || size == 0) return false;
    if (addr >= kGuestSize) return false;
    if (addr + size > kGuestSize) size = kGuestSize - addr;
#if defined(_WIN32)
    // Commit page-aligned region; already-committed pages are fine
    uint64_t page = addr & ~0xFFFFull;
    uint64_t end  = (addr + size + 0xFFFF) & ~0xFFFFull;
    return VirtualAlloc(g_mem + page, (SIZE_T)(end - page), MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
    (void)addr; (void)size; return true;
#endif
}
} // namespace

PS3RT_API void ps3rt_touch(uint64_t addr, size_t len) {
    if (!g_mem || len == 0) return;
    if (addr >= kGuestSize) return;
    map_region(addr, len);
}

PS3RT_API int ps3rt_init(const char* image_path) {
    FILE* f = std::fopen(image_path, "rb");
    if (!f) return 1;
    char magic[8]; uint32_t nseg = 0;
    if (std::fread(magic, 1, 8, f) != 8 || std::memcmp(magic, "PS3IMG1\0", 8) != 0 || std::fread(&nseg, 4, 1, f) != 1) {
        std::fclose(f); return 2;
    }
#if defined(_WIN32)
    g_mem = (uint8_t*)VirtualAlloc(nullptr, (SIZE_T)kGuestSize, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* p = mmap(nullptr, (size_t)kGuestSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    g_mem = (p == MAP_FAILED) ? nullptr : (uint8_t*)p;
#endif
    if (!g_mem) { std::fclose(f); return 3; }
    struct Seg { uint64_t vaddr, filesz, memsz, off; };
    std::vector<Seg> segs(nseg);
    if (nseg && std::fread(segs.data(), sizeof(Seg), nseg, f) != nseg) { std::fclose(f); return 2; }
    for (const Seg& s : segs) {
        if (s.vaddr >= kGuestSize) continue;
        uint64_t memsz = s.memsz;
        if (s.vaddr + memsz > kGuestSize) memsz = kGuestSize - s.vaddr;
        map_region(s.vaddr, (memsz + 0xFFFF) & ~0xFFFFull);
        std::fseek(f, (long)s.off, SEEK_SET);
        uint64_t filesz = s.filesz < memsz ? s.filesz : memsz;
        if (filesz && std::fread(g_mem + s.vaddr, 1, (size_t)filesz, f) != filesz) { std::fclose(f); return 2; }
    }
    std::fclose(f);
    map_region(kStackBase, kStackSize);
    map_region(kHeapBase, kHeapSize);
    g_heap_next = kHeapBase;
    std::memset(g_spu_used, 0, sizeof g_spu_used);
    std::fprintf(stderr, "[ps3rt] image loaded, segments=%u stack=0x%llx heap=0x%llx\n",
        nseg, (unsigned long long)kStackBase, (unsigned long long)kHeapBase);
    return 0;
}

PS3RT_API uint8_t* ps3rt_memory(void)    { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize - 0x100; }
PS3RT_API void ps3rt_shutdown(void) {
#if defined(_WIN32)
    if (g_mem) VirtualFree(g_mem, 0, MEM_RELEASE);
#else
    if (g_mem) munmap(g_mem, (size_t)kGuestSize);
#endif
    g_mem = nullptr;
}

// ---- SPU ----
PS3RT_API int ps3rt_spu_supported(void) { return 1; }

PS3RT_API int ps3rt_spu_create(int* out_id) {
    std::lock_guard<std::mutex> lock(g_spu_mu);
    for (int i = 0; i < kMaxSpu; ++i) {
        if (!g_spu_used[i]) {
            g_spu_used[i] = true;
            std::memset(&g_spu[i], 0, sizeof(SPUContext));
            if (out_id) *out_id = i;
            std::fprintf(stderr, "[spu] create id=%d\n", i);
            return 0;
        }
    }
    return -1;
}

PS3RT_API int ps3rt_spu_destroy(int id) {
    if (id < 0 || id >= kMaxSpu) return -1;
    std::lock_guard<std::mutex> lock(g_spu_mu);
    g_spu_used[id] = false;
    g_spu[id].running = false;
    return 0;
}

PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !img) return -1;
    if (len > sizeof(g_spu[id].ls)) len = sizeof(g_spu[id].ls);
    std::memcpy(g_spu[id].ls, img, len);
    std::fprintf(stderr, "[spu] load id=%d bytes=%zu\n", id, len);
    return 0;
}

PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    g_spu[id].pc = entry & ~3u;
    g_spu[id].running = true;
    // Stub interpreter: drain a few "instructions" then stop (full SPU ISA later)
    for (int step = 0; step < 256 && g_spu[id].running; ++step) {
        uint32_t* w = (uint32_t*)(g_spu[id].ls + (g_spu[id].pc & 0x3FFFC));
        uint32_t insn = bs32(*w);
        // stop / stopd encoding roughly 0x00000000 family and 0x3fffxxxx
        if ((insn >> 21) == 0 || (insn & 0xFF800000u) == 0x00000000u) {
            g_spu[id].running = false;
            break;
        }
        // nop-ish advance
        g_spu[id].pc = (g_spu[id].pc + 4) & 0x3FFFF;
    }
    g_spu[id].running = false;
    std::fprintf(stderr, "[spu] run id=%d entry=0x%x done\n", id, entry);
    return 0;
}

PS3RT_API int ps3rt_spu_stop(int id) {
    if (id < 0 || id >= kMaxSpu) return -1;
    g_spu[id].running = false;
    return 0;
}

PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id]) return -1;
    if (g_spu[id].mailbox_in_count >= 4) return -1;
    g_spu[id].mailbox_in[g_spu[id].mailbox_in_count++] = val;
    return 0;
}

PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !out) return -1;
    if (g_spu[id].mailbox_out_count <= 0) { *out = 0; return -1; }
    *out = g_spu[id].mailbox_out[0];
    for (int i = 1; i < g_spu[id].mailbox_out_count; ++i)
        g_spu[id].mailbox_out[i - 1] = g_spu[id].mailbox_out[i];
    --g_spu[id].mailbox_out_count;
    return 0;
}

PS3RT_API int ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd) {
    if (id < 0 || id >= kMaxSpu || !g_spu_used[id] || !g_mem) return -1;
    lsa &= 0x3FFFF;
    if ((uint64_t)lsa + size > sizeof(g_spu[id].ls)) return -1;
    if (ea >= kGuestSize) return -1;
    map_region(ea, size);
    // cmd low bits: 0x20 get, 0x40 put (simplified)
    if (cmd & 0x20) {
        std::memcpy(g_spu[id].ls + lsa, g_mem + ea, size);
    } else if (cmd & 0x40) {
        std::memcpy(g_mem + ea, g_spu[id].ls + lsa, size);
    } else {
        // default: get
        std::memcpy(g_spu[id].ls + lsa, g_mem + ea, size);
    }
    return 0;
}

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    uint64_t n = c->gpr[11];
    switch (n) {
    case 1:  // sys_process_exit
    case 22:
    case 41:
        c->halted = true; break;

    case 348: // sys_memory_allocate (simplified)
    case 352: {
        uint64_t size = c->gpr[3];
        if (size == 0) size = 0x10000;
        size = (size + 0xFFFF) & ~0xFFFFull;
        if (g_heap_next + size > kHeapBase + kHeapSize) {
            c->gpr[3] = 0x80010005ull; // ENOMEM-ish
            break;
        }
        uint64_t addr = g_heap_next;
        g_heap_next += size;
        map_region(addr, size);
        if (c->gpr[5]) wr64(*c, c->gpr[5], addr);
        c->gpr[3] = 0;
        break;
    }

    // SPU thread group / thread syscalls (stubs that succeed)
    case 170: // sys_spu_thread_group_create
    case 171: // sys_spu_thread_group_destroy
    case 172: // sys_spu_thread_group_start
    case 173: // sys_spu_thread_group_join
    case 182: // sys_spu_thread_initialize
    case 183: // sys_spu_thread_set_argument
    case 185: // sys_spu_thread_group_connect_event
    case 190: // sys_spu_thread_write_ls
    case 191: // sys_spu_thread_read_ls
    case 193: // sys_spu_thread_group_connect_mfc_event
    {
        int sid = -1;
        if (n == 182 || n == 170) {
            ps3rt_spu_create(&sid);
            if (c->gpr[3] && sid >= 0) wr32(*c, c->gpr[3], (uint32_t)sid);
        }
        c->gpr[3] = 0;
        break;
    }

    case 403: { // sys_tty_write
        uint64_t buf = c->gpr[4], len = c->gpr[5];
        if (buf < kGuestSize && len && g_mem) {
            map_region(buf, (size_t)len);
            std::fwrite(g_mem + buf, 1, (size_t)len, stdout);
            std::fflush(stdout);
        }
        if (c->gpr[6]) wr32(*c, c->gpr[6], (uint32_t)len);
        c->gpr[3] = 0; break;
    }

    default:
        // Many LV2 calls expect 0 (CELL_OK)
        c->gpr[3] = 0;
        break;
    }
}

PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    static int count = 0;
    if (count < 20) {
        std::fprintf(stderr,
            "[ps3rt] unimplemented insn 0x%08x at pc=0x%llx\n"
            "  lr=0x%llx ctr=0x%llx r1=0x%llx r2=0x%llx r3=0x%llx\n",
            opcode, (unsigned long long)pc,
            (unsigned long long)c->lr, (unsigned long long)c->ctr,
            (unsigned long long)c->gpr[1], (unsigned long long)c->gpr[2],
            (unsigned long long)c->gpr[3]);
        ++count;
    }
}
