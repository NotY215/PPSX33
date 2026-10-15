// ps3rt Phase XVI-XVIII — audio/save (audio_save.cpp) + pad/input
#define PS3RT_BUILD_DLL 1
#include "ppu_runtime.h"
#include "nid_table.h"
#include "ps3rt_stage.h"
extern "C" {
PS3RT_API int  ps3rt_audio_dispatch(PPUContext* c, uint32_t nid);
PS3RT_API void ps3rt_audio_tick(void);
PS3RT_API int  ps3rt_save_dispatch(PPUContext* c, uint32_t nid);
PS3RT_API void ps3rt_save_boot_notice(void);
PS3RT_API void ps3rt_assets_boot_scan(void);
}
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <algorithm>
#include <vector>
#include <cstdarg>
#include <ctime>
#include <atomic>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static FILE* g_logf = nullptr;
static void ps3rt_log_close(void);
static std::atomic<int> g_exit_requested{0};
PS3RT_API void ps3rt_request_exit(void) {
    g_exit_requested.store(1);
    PS3RT_STAGE("EXIT requested");
    ps3rt_log("[ps3rt] exit requested\n");
}
PS3RT_API int ps3rt_should_exit(void) { return g_exit_requested.load() != 0; }
PS3RT_API void ps3rt_force_exit(void) {
    g_exit_requested.store(1);
    ps3rt_log("[ps3rt] force exit\n");
    ps3rt_log_close();
#if defined(_WIN32)
    ExitProcess(0);
#else
    std::_Exit(0);
#endif
}
static void ps3rt_log_close(void) {
    if (g_logf) { std::fprintf(g_logf, "\n--- log end ---\n"); std::fflush(g_logf); std::fclose(g_logf); g_logf = nullptr; }
}
#if defined(_WIN32)
static int path_looks_like_game_folder(const char* dir) {
    if (!dir || !dir[0]) return 0;
    char probe[640];
    const char* checks[] = {
        "USRDIR", "PS3_GAME", "EBOOT.BIN", "PARAM.SFO", "PS3_DISC.SFB",
        "USRDIR\\EBOOT.BIN", "PS3_GAME\\USRDIR", nullptr
    };
    for (int i = 0; checks[i]; ++i) {
        std::snprintf(probe, sizeof(probe), "%s%s", dir, checks[i]);
        DWORD a = GetFileAttributesA(probe);
        if (a != INVALID_FILE_ATTRIBUTES) return 1;
    }
    return 0;
}
#else
static int path_looks_like_game_folder(const char*) { return 0; }
#endif

static void ps3rt_log_open(void) {
    if (g_logf) return;
    char dir[512]; dir[0] = 0;
#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(nullptr, dir, (DWORD)sizeof(dir));
    if (n && n < sizeof(dir)) {
        char* slash = std::strrchr(dir, '\\');
        if (!slash) slash = std::strrchr(dir, '/');
        if (slash) *(slash + 1) = 0; else std::snprintf(dir, sizeof(dir), ".\\");
    } else std::snprintf(dir, sizeof(dir), ".\\");
#else
    std::snprintf(dir, sizeof(dir), "./");
#endif
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    int in_game = path_looks_like_game_folder(dir);
    char name[96];
    if (in_game) {
        std::snprintf(name, sizeof(name),
            "ppsx33_log_in_game_folder_%04d-%02d-%02d_%02d-%02d-%02d.txt",
            tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
            tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    } else {
        std::snprintf(name, sizeof(name),
            "ppsx33_log_outside_game_%04d-%02d-%02d_%02d-%02d-%02d.txt",
            tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
            tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    }
    char full[600];
    std::snprintf(full, sizeof(full), "%s%s", dir, name);
    g_logf = std::fopen(full, "w");
    if (g_logf) {
        std::fprintf(g_logf,
            "PPSX33 run log\n"
            "mode=%s\n"
            "file=%s\n",
            in_game ? "launch_in_game_folder" : "launch_outside_game",
            full);
        std::fflush(g_logf);
        ps3rt_log("[ps3rt] log file (%s): %s\n",
            in_game ? "in_game_folder" : "outside_game", full);
    }
}
PS3RT_API void ps3rt_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fflush(stderr);
    if (g_logf) {
        va_start(ap, fmt);
        std::vfprintf(g_logf, fmt, ap);
        va_end(ap);
        std::fflush(g_logf);
    }
}

static uint8_t* g_mem = nullptr;
static size_t g_mem_size = 0;
static const uint64_t kStackBase = 0x0F000000ull;
static const size_t kStackSize = 0x100000;
static std::mutex g_lock;
static uint64_t g_alloc_ptr = 0x08000000ull;
static const uint64_t kAllocEnd = 0x0E000000ull;
static void wr_be64_guest(uint64_t addr, uint64_t val) {
    if (!g_mem || addr + 8 > g_mem_size) return;
    uint8_t b[8]; for (int i = 7; i >= 0; --i) { b[i] = (uint8_t)(val & 0xFF); val >>= 8; }
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
    if (a + size > kAllocEnd || a + size > g_mem_size) return 0;
    std::memset(g_mem + a, 0, (size_t)size);
    g_alloc_ptr = a + size;
    ps3rt_log("[lv2] alloc VA=0x%llx size=0x%llx\n", (unsigned long long)a, (unsigned long long)size);
    return a;
}
PS3RT_API void ps3rt_touch(uint64_t, size_t) {
    static uint32_t s_n = 0;
    static uint32_t s_log = 0;
    if ((++s_n & 0x1FFF) == 0) {
        ps3rt_progress_hw();
        if (s_log < 12) { ps3rt_log("[ps3] Phase M touch-progress n=%u\n", s_n); ++s_log; }
    }
}
static uint32_t rd_le32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
static uint64_t rd_le64(const uint8_t* p) { uint64_t v; std::memcpy(&v, p, 8); return v; }
static int load_guest_image(const char* path) {
    FILE* f = std::fopen(path, "rb"); if (!f) return -1;
    std::fseek(f, 0, SEEK_END); long fsz = std::ftell(f); std::fseek(f, 0, SEEK_SET);
    if (fsz < 12) { std::fclose(f); return -1; }
    std::vector<uint8_t> file((size_t)fsz);
    if (std::fread(file.data(), 1, (size_t)fsz, f) != (size_t)fsz) { std::fclose(f); return -1; }
    std::fclose(f);
    if (std::memcmp(file.data(), "PS3IMG1", 7) == 0) {
        uint32_t nseg = rd_le32(file.data() + 8);
        int mapped = 0;
        for (uint32_t i = 0; i < nseg; ++i) {
            const uint8_t* e = file.data() + 12 + i * 32;
            uint64_t vaddr = rd_le64(e + 0), filesz = rd_le64(e + 8), memsz = rd_le64(e + 16), off = rd_le64(e + 24);
            if (filesz == 0 && memsz == 0) continue;
            if (off + filesz > (size_t)fsz) continue;
            if (vaddr + memsz > g_mem_size) { if (vaddr >= g_mem_size) continue; memsz = g_mem_size - vaddr; if (filesz > memsz) filesz = memsz; }
            std::memset(g_mem + vaddr, 0, (size_t)memsz);
            if (filesz) std::memcpy(g_mem + vaddr, file.data() + (size_t)off, (size_t)filesz);
            ps3rt_log("[ps3rt] map seg%u VA=0x%llx filesz=0x%llx memsz=0x%llx\n", i, (unsigned long long)vaddr, (unsigned long long)filesz, (unsigned long long)memsz);
            ++mapped;
        }
        ps3rt_log("[ps3rt] PS3IMG1 loaded from %s (%d segments)\n", path, mapped);
        PS3RT_STAGE("MEM map end");
        return mapped > 0 ? 0 : -1;
    }
    if ((size_t)fsz <= g_mem_size) { std::memcpy(g_mem, file.data(), (size_t)fsz); return 0; }
    return -1;
}

PS3RT_API int ps3rt_init(const char* image_path) {
    std::lock_guard<std::mutex> lk(g_lock);
    ps3rt_log_open();
    g_exit_requested.store(0);
    PS3RT_STAGE("BOOT start");
    if (g_mem) return 0;
    g_mem_size = 512ull * 1024 * 1024;
    g_mem = (uint8_t*)std::malloc(g_mem_size);
    if (!g_mem) return -1;
    std::memset(g_mem, 0, g_mem_size);
    g_alloc_ptr = 0x08000000ull;
    const char* candidates[] = { image_path, "guest_image.bin", "output/guest_image.bin" };
    int ok = -1;
    for (auto c : candidates) {
        if (c && c[0] && load_guest_image(c) == 0) { ok = 0; break; }
    }
    if (ok != 0) ps3rt_log("[ps3rt] warning: no guest_image.bin mapped\n");
    ps3rt_save_boot_notice();
    ps3rt_assets_boot_scan();
    PS3RT_STAGE("BOOT end");
    return 0;
}
PS3RT_API uint8_t* ps3rt_memory(void) { return g_mem; }
PS3RT_API uint64_t ps3rt_stack_top(void) { return kStackBase + kStackSize; }
PS3RT_API void ps3rt_shutdown(void) { std::lock_guard<std::mutex> lk(g_lock); std::free(g_mem); g_mem = nullptr; g_mem_size = 0; ps3rt_log_close(); }

SPUContext g_spus[8];
int g_spu_count = 0;
PS3RT_API int ps3rt_spu_supported(void) { return 1; }
PS3RT_API int ps3rt_spu_create(int* out_id) {
    if (g_spu_count >= 8) return -1;
    int id = g_spu_count++;
    std::memset(&g_spus[id], 0, sizeof(SPUContext));
    g_spus[id].spurs_idle = true;
    g_spus[id].spurs_group = -1;
    if (out_id) *out_id = id;
    return 0;
}
PS3RT_API int ps3rt_spu_destroy(int id) { if (id < 0 || id >= g_spu_count) return -1; g_spus[id].running = false; return 0; }
PS3RT_API int ps3rt_spu_load(int id, const void* img, size_t len) {
    if (id < 0 || id >= g_spu_count || !img) return -1;
    size_t n = len < sizeof(g_spus[id].ls) ? len : sizeof(g_spus[id].ls);
    std::memcpy(g_spus[id].ls, img, n);
    return 0;
}
// Declared in spu_stub.cpp (XX-K interpreter)
extern "C" void ps3rt_spu_interpreter_start(int id, uint32_t entry);

PS3RT_API int ps3rt_spu_run(int id, uint32_t entry) {
    if (id < 0 || id >= g_spu_count) return -1;
    ps3rt_spu_interpreter_start(id, entry);
    // If already stopped after initial slice, mbox was posted by stop handler
    if (!g_spus[id].running && g_spus[id].mailbox_out_count == 0) {
        g_spus[id].mailbox_out[g_spus[id].mailbox_out_count++] = 1;
    }
    return 0;
}
PS3RT_API int ps3rt_spu_stop(int id) { if (id < 0 || id >= g_spu_count) return -1; g_spus[id].running = false; return 0; }
PS3RT_API int ps3rt_spu_mbox_write(int id, uint32_t val) {
    if (id < 0 || id >= g_spu_count) return -1;
    if (g_spus[id].mailbox_in_count >= 4) return -1;
    g_spus[id].mailbox_in[g_spus[id].mailbox_in_count++] = val; return 0;
}
PS3RT_API int ps3rt_spu_mbox_read(int id, uint32_t* out) {
    if (id < 0 || id >= g_spu_count || !out) return -1;
    if (g_spus[id].mailbox_out_count <= 0) { *out = 0; return -1; }
    *out = g_spus[id].mailbox_out[0];
    for (int i = 1; i < g_spus[id].mailbox_out_count; ++i) g_spus[id].mailbox_out[i-1] = g_spus[id].mailbox_out[i];
    --g_spus[id].mailbox_out_count; return 0;
}
PS3RT_API int ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd) {
    if (id < 0 || id >= g_spu_count) return -1;
    SPUContext& s = g_spus[id];
    s.mfc_lsa = lsa;
    s.mfc_ea = ea;
    s.mfc_size = size;
    s.mfc_cmd = cmd;
    // Reuse interpreter DMA path via channel write simulation
    // Inline minimal put/get here for PPU-initiated DMA
    lsa &= 0x3FFFF;
    if (!g_mem || size == 0) return 0;
    if (ea + size > g_mem_size) {
        if (ea >= g_mem_size) return -1;
        size = (uint32_t)(g_mem_size - ea);
    }
    if ((uint64_t)lsa + size > sizeof(s.ls)) size = (uint32_t)(sizeof(s.ls) - lsa);
    bool is_get = (cmd == 0x20 || cmd == 0x21 || cmd == 0x22 || cmd == 0x30 || cmd == 0x31 || cmd == 0x32);
    if (is_get) std::memcpy(s.ls + lsa, g_mem + ea, size);
    else std::memcpy(g_mem + ea, s.ls + lsa, size);
    s.mfc_tag_status |= (1u << (s.mfc_tag & 31));
    return 0;
}
extern "C" PS3RT_API void ps3rt_spu_tick_all(void);

static void spu_tick_all() {
    static int s_spu_once = 0;
    if (!s_spu_once) { PS3RT_STAGE("SPU progress end"); s_spu_once = 1; }
    ps3rt_spu_tick_all();
}

// XXII-K + Phase XVII: full pad / input mapping ----
enum : uint16_t {
    CELL_PAD_SELECT   = 0x0001,
    CELL_PAD_L3       = 0x0002,
    CELL_PAD_R3       = 0x0004,
    CELL_PAD_START    = 0x0008,
    CELL_PAD_UP       = 0x0010,
    CELL_PAD_RIGHT    = 0x0020,
    CELL_PAD_DOWN     = 0x0040,
    CELL_PAD_LEFT     = 0x0080,
    CELL_PAD_L2       = 0x0100,
    CELL_PAD_R2       = 0x0200,
    CELL_PAD_L1       = 0x0400,
    CELL_PAD_R1       = 0x0800,
    CELL_PAD_TRIANGLE = 0x1000,
    CELL_PAD_CIRCLE   = 0x2000,
    CELL_PAD_CROSS    = 0x4000,
    CELL_PAD_SQUARE   = 0x8000,
};
static uint16_t g_pad_buttons = 0;
static int8_t g_pad_lx = 0, g_pad_ly = 0, g_pad_rx = 0, g_pad_ry = 0;
static uint8_t g_pad_l2 = 0, g_pad_r2 = 0;
// Q: pressure (0-255) + sixaxis accel stubs
static uint8_t g_pad_pressure[16] = {};
static int16_t g_pad_accel_x = 0, g_pad_accel_y = 0, g_pad_accel_z = 0;
static int g_pad_sensor_enabled = 0;
static int g_pad_inited = 0;
static int g_pad_connected = 1;
static uint32_t g_pad_poll_count = 0;

PS3RT_API void ps3rt_pad_set(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry) {
    g_pad_buttons = buttons;
    g_pad_lx = (int8_t)(lx >> 8); g_pad_ly = (int8_t)(ly >> 8);
    g_pad_rx = (int8_t)(rx >> 8); g_pad_ry = (int8_t)(ry >> 8);
}
PS3RT_API void ps3rt_pad_set_sensor(int enabled) {
    g_pad_sensor_enabled = enabled ? 1 : 0;
    static int s_q = 0;
    if (s_q < 4) { ps3rt_log("[pad] Q sensor %s\n", enabled ? "on" : "off"); ++s_q; }
}
PS3RT_API void ps3rt_pad_get_pressure(uint8_t out[16]) {
    if (!out) return;
    for (int i = 0; i < 16; ++i) out[i] = g_pad_pressure[i];
}
PS3RT_API void ps3rt_pad_get_accel(int16_t* x, int16_t* y, int16_t* z) {
    if (x) *x = g_pad_accel_x;
    if (y) *y = g_pad_accel_y;
    if (z) *z = g_pad_accel_z;
}

#if defined(_WIN32)
typedef struct _XINPUT_GAMEPAD {
    WORD wButtons; BYTE bLeftTrigger; BYTE bRightTrigger;
    SHORT sThumbLX; SHORT sThumbLY; SHORT sThumbRX; SHORT sThumbRY;
} XINPUT_GAMEPAD;
typedef struct _XINPUT_STATE { DWORD dwPacketNumber; XINPUT_GAMEPAD Gamepad; } XINPUT_STATE;
typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XINPUT_STATE*);
static PFN_XInputGetState s_XInputGetState = nullptr;
static int s_xinput_tried = 0;
static void pad_load_xinput() {
    if (s_xinput_tried) return;
    s_xinput_tried = 1;
    const char* dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll", nullptr };
    for (int i = 0; dlls[i]; ++i) {
        HMODULE m = LoadLibraryA(dlls[i]);
        if (!m) continue;
        s_XInputGetState = (PFN_XInputGetState)GetProcAddress(m, "XInputGetState");
        if (s_XInputGetState) {
            ps3rt_log("[pad] XInput loaded from %s\n", dlls[i]);
            return;
        }
    }
    ps3rt_log("[pad] XInput not found — keyboard only\n");
}
static int16_t stick_from_xinput(SHORT v) {
    if (v > -8000 && v < 8000) return 0;
    int scaled = (int)v * 127 / 32767;
    if (scaled > 127) scaled = 127;
    if (scaled < -128) scaled = -128;
    return (int16_t)scaled;
}
#endif


static int window_is_focused(void) {
#if defined(_WIN32)
    HWND fg = GetForegroundWindow();
    if (!fg) return 0;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    return pid == GetCurrentProcessId() ? 1 : 0;
#else
    return 1;
#endif
}

static void pad_poll_host() {
    ++g_pad_poll_count;
    uint16_t btn = 0;
    int8_t lx = 0, ly = 0, rx = 0, ry = 0;
    uint8_t l2 = 0, r2 = 0;

#if defined(_WIN32)
    pad_load_xinput();
    if (s_XInputGetState) {
        XINPUT_STATE st{};
        if (s_XInputGetState(0, &st) == 0) {
            WORD xb = st.Gamepad.wButtons;
            if (xb & 0x0001) btn |= CELL_PAD_UP;
            if (xb & 0x0002) btn |= CELL_PAD_DOWN;
            if (xb & 0x0004) btn |= CELL_PAD_LEFT;
            if (xb & 0x0008) btn |= CELL_PAD_RIGHT;
            if (xb & 0x0010) btn |= CELL_PAD_START;
            if (xb & 0x0020) btn |= CELL_PAD_SELECT;
            if (xb & 0x0040) btn |= CELL_PAD_L3;
            if (xb & 0x0080) btn |= CELL_PAD_R3;
            if (xb & 0x0100) btn |= CELL_PAD_L1;
            if (xb & 0x0200) btn |= CELL_PAD_R1;
            if (xb & 0x1000) btn |= CELL_PAD_CROSS;
            if (xb & 0x2000) btn |= CELL_PAD_CIRCLE;
            if (xb & 0x4000) btn |= CELL_PAD_SQUARE;
            if (xb & 0x8000) btn |= CELL_PAD_TRIANGLE;
            l2 = st.Gamepad.bLeftTrigger;
            r2 = st.Gamepad.bRightTrigger;
            if (l2 > 30) btn |= CELL_PAD_L2;
            if (r2 > 30) btn |= CELL_PAD_R2;
            lx = (int8_t)stick_from_xinput(st.Gamepad.sThumbLX);
            ly = (int8_t)(-stick_from_xinput(st.Gamepad.sThumbLY));
            rx = (int8_t)stick_from_xinput(st.Gamepad.sThumbRX);
            ry = (int8_t)(-stick_from_xinput(st.Gamepad.sThumbRY));
            g_pad_connected = 1;
        }
    }
    #define PAD_KEY(vk) ((GetAsyncKeyState(vk) & 0x8000) != 0)
    if (PAD_KEY(VK_UP)    || PAD_KEY('W')) btn |= CELL_PAD_UP;
    if (PAD_KEY(VK_DOWN)  || PAD_KEY('S')) btn |= CELL_PAD_DOWN;
    if (PAD_KEY(VK_LEFT)  || PAD_KEY('A')) btn |= CELL_PAD_LEFT;
    if (PAD_KEY(VK_RIGHT) || PAD_KEY('D')) btn |= CELL_PAD_RIGHT;
    if (PAD_KEY(VK_RETURN) || PAD_KEY('K')) btn |= CELL_PAD_CROSS;
    if (PAD_KEY(VK_ESCAPE) || PAD_KEY('L')) btn |= CELL_PAD_CIRCLE;
    if (PAD_KEY(VK_SPACE)  || PAD_KEY('I')) btn |= CELL_PAD_TRIANGLE;
    if (PAD_KEY(VK_BACK)   || PAD_KEY('J')) btn |= CELL_PAD_SQUARE;
    if (PAD_KEY('Q')) btn |= CELL_PAD_L1;
    if (PAD_KEY('E')) btn |= CELL_PAD_R1;
    if (PAD_KEY('Z')) { btn |= CELL_PAD_L2; l2 = 255; }
    if (PAD_KEY('C')) { btn |= CELL_PAD_R2; r2 = 255; }
    if (PAD_KEY(VK_TAB))   btn |= CELL_PAD_START;
    if (PAD_KEY(VK_SHIFT)) btn |= CELL_PAD_SELECT;
    if (!(lx|ly)) {
        if (PAD_KEY('W')) ly = -127;
        if (PAD_KEY('S')) ly =  127;
        if (PAD_KEY('A')) lx = -127;
        if (PAD_KEY('D')) lx =  127;
    }
    #undef PAD_KEY
#endif

    g_pad_buttons = btn;
    g_pad_lx = lx; g_pad_ly = ly; g_pad_rx = rx; g_pad_ry = ry;
    g_pad_l2 = l2; g_pad_r2 = r2;
    // Q: map digital face buttons to full pressure when held
    g_pad_pressure[0] = (btn & CELL_PAD_LEFT) ? 255 : 0;
    g_pad_pressure[1] = (btn & CELL_PAD_DOWN) ? 255 : 0;
    g_pad_pressure[2] = (btn & CELL_PAD_RIGHT) ? 255 : 0;
    g_pad_pressure[3] = (btn & CELL_PAD_UP) ? 255 : 0;
    g_pad_pressure[4] = (btn & CELL_PAD_SQUARE) ? 255 : 0;
    g_pad_pressure[5] = (btn & CELL_PAD_CROSS) ? 255 : 0;
    g_pad_pressure[6] = (btn & CELL_PAD_CIRCLE) ? 255 : 0;
    g_pad_pressure[7] = (btn & CELL_PAD_TRIANGLE) ? 255 : 0;
    g_pad_pressure[8] = (btn & CELL_PAD_R1) ? 255 : 0;
    g_pad_pressure[9] = (btn & CELL_PAD_L1) ? 255 : 0;
    g_pad_pressure[10] = r2;
    g_pad_pressure[11] = l2;
    // sixaxis: neutral when sensor off; mild gravity on Z when on
    if (g_pad_sensor_enabled) {
        g_pad_accel_x = 0; g_pad_accel_y = 0; g_pad_accel_z = 256;
    } else {
        g_pad_accel_x = g_pad_accel_y = g_pad_accel_z = 0;
    }
    // Only log pad/XInput while the game window is focused
    if (window_is_focused()) {
        if ((g_pad_poll_count <= 4) || ((g_pad_poll_count & 0x3FF) == 1 && btn)) {
            ps3rt_log("[pad] poll#%u btn=0x%04X sticks=%d,%d,%d,%d L2=%u R2=%u\n",
                g_pad_poll_count, (unsigned)btn, (int)lx, (int)ly, (int)rx, (int)ry, (unsigned)l2, (unsigned)r2);
        }
    }
}

#include "ps3rt_b.inl"
