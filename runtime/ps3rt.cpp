// ps3rt - runtime library (ps3rt.dll)
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include <cstdio>
#include <cstring>
#include <vector>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/mman.h>
#endif

namespace {
constexpr uint64_t kGuestSize = 1ull << 32;
constexpr uint64_t kStackBase = 0xD0000000ull;
constexpr uint64_t kStackSize = 1ull << 20;
uint8_t* g_mem = nullptr;

bool map_region(uint64_t addr, uint64_t size) {
#if defined(_WIN32)
    return VirtualAlloc(g_mem + addr, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
    (void)addr; (void)size; return true;
#endif
}
}

PS3RT_API int ps3rt_init(const char* image_path) {
    FILE* f = std::fopen(image_path, "rb");
    if (!f) return 1;
    char magic[8]; uint32_t nseg = 0;
    if (std::fread(magic, 1, 8, f) != 8 || std::memcmp(magic, "PS3IMG1\0", 8) != 0 || std::fread(&nseg, 4, 1, f) != 1) {
        std::fclose(f); return 2;
    }
#if defined(_WIN32)
    g_mem = (uint8_t*)VirtualAlloc(nullptr, kGuestSize, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* p = mmap(nullptr, kGuestSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    g_mem = (p == MAP_FAILED) ? nullptr : (uint8_t*)p;
#endif
    if (!g_mem) { std::fclose(f); return 3; }
    struct Seg { uint64_t vaddr, filesz, memsz, off; };
    std::vector<Seg> segs(nseg);
    if (nseg && std::fread(segs.data(), sizeof(Seg), nseg, f) != nseg) { std::fclose(f); return 2; }
    for (const Seg& s : segs) {
        if (s.vaddr + s.memsz > kGuestSize) { std::fclose(f); return 4; }
        map_region(s.vaddr, (s.memsz + 0xFFFF) & ~0xFFFFull);
        std::fseek(f, (long)s.off, SEEK_SET);
        if (s.filesz && std::fread(g_mem + s.vaddr, 1, (size_t)s.filesz, f) != s.filesz) { std::fclose(f); return 2; }
    }
    std::fclose(f);
    map_region(kStackBase, kStackSize);
    return 0;
}

PS3RT_API uint8_t* ps3rt_memory(void)    { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize - 0x100; }
PS3RT_API void ps3rt_shutdown(void) {
#if defined(_WIN32)
    if (g_mem) VirtualFree(g_mem, 0, MEM_RELEASE);
#else
    if (g_mem) munmap(g_mem, kGuestSize);
#endif
    g_mem = nullptr;
}

PS3RT_API void ps3rt_syscall(PPUContext* c) {
    uint64_t n = c->gpr[11];
    switch (n) {
    case 22:
    case 41:
        c->halted = true; break;
    case 403: {
        uint64_t buf = c->gpr[4], len = c->gpr[5];
        std::fwrite(c->mem + buf, 1, (size_t)len, stdout);
        std::fflush(stdout);
        if (c->gpr[6]) wr32(*c, c->gpr[6], (uint32_t)len);
        c->gpr[3] = 0; break;
    }
    default:
        std::fprintf(stderr, "[ps3rt] syscall %llu -> CELL_OK (stub)\n", (unsigned long long)n);
        c->gpr[3] = 0; break;
    }
}

PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc) {
    std::fprintf(stderr,
        "[ps3rt] unimplemented insn 0x%08x at pc=0x%llx\n"
        "  lr=0x%llx ctr=0x%llx r1=0x%llx r2=0x%llx r3=0x%llx\n",
        opcode, (unsigned long long)pc,
        (unsigned long long)c->lr, (unsigned long long)c->ctr,
        (unsigned long long)c->gpr[1], (unsigned long long)c->gpr[2],
        (unsigned long long)c->gpr[3]);
}
