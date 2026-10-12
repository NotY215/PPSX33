// ppu_runtime_core_b.h - continuation of core (rd helpers, CR/branch, chunks, API)
// Appended after core_a
// NOTE: SPUContext is defined in core_a.h — do not redeclare

static inline void set_cr_signed(PPUContext& c, int f, int64_t a, int64_t b) {
    uint8_t bits = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
    if (f == 0 && xer_so(c)) bits |= 1;
    c.cr[f] = bits;
}
static inline void set_cr_unsigned(PPUContext& c, int f, uint64_t a, uint64_t b) {
    uint8_t bits = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
    if (f == 0 && xer_so(c)) bits |= 1;
    c.cr[f] = bits;
}
static inline void set_cr_fp(PPUContext& c, int f, double a, double b) {
    if (std::isnan(a) || std::isnan(b)) { c.cr[f] = 1; return; }
    c.cr[f] = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
}

#ifndef PS3RT_API
#  if defined(_WIN32) && defined(PS3RT_BUILD_DLL)
#    define PS3RT_API extern "C" __declspec(dllexport)
#  elif defined(_WIN32)
#    define PS3RT_API extern "C" __declspec(dllimport)
#  else
#    define PS3RT_API extern "C"
#  endif
#endif
PS3RT_API void ps3rt_progress_hw(void);
PS3RT_API void ps3rt_request_exit(void);
PS3RT_API int  ps3rt_should_exit(void);

// Phase M: force fallthrough after 2048 consecutive taken (was 65536)
static inline bool branch_taken_bo(PPUContext& c, int bo, int bi) {
    bool ctr_ok = true;
    if ((bo & 4) == 0) {
        c.ctr--;
        ctr_ok = ((bo & 2) != 0) ? (c.ctr == 0) : (c.ctr != 0);
    }
    bool cond_ok = true;
    if ((bo & 16) == 0) {
        int crb = (bi >> 2) & 7;
        int bit = 3 - (bi & 3);
        bool v = (c.cr[crb] & (1u << bit)) != 0;
        cond_ok = ((bo & 8) != 0) ? v : !v;
    }
    return ctr_ok && cond_ok;
}

struct PPUChunk {
    uint64_t start;
    uint64_t end;
    void (*fn)(PPUContext&);
};

PS3RT_API void ps3rt_syscall(PPUContext* c);
PS3RT_API int  ps3rt_prx_import_stub(PPUContext* c, uint64_t pc);
PS3RT_API void ps3rt_pad_set(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry);
PS3RT_API void ps3rt_unimplemented(PPUContext* c, uint32_t opcode, uint64_t pc);
PS3RT_API int      ps3rt_init(const char* image_path);
PS3RT_API uint8_t* ps3rt_memory(void);
PS3RT_API uint64_t ps3rt_stack_top(void);
PS3RT_API void     ps3rt_shutdown(void);
PS3RT_API int  ps3rt_spu_supported(void);
PS3RT_API int  ps3rt_spu_create(int* out_id);
PS3RT_API int  ps3rt_spu_destroy(int id);
PS3RT_API int  ps3rt_spu_load(int id, const void* img, size_t len);
PS3RT_API int  ps3rt_spu_run(int id, uint32_t entry);
PS3RT_API int  ps3rt_spu_stop(int id);
PS3RT_API int  ps3rt_spu_mbox_write(int id, uint32_t val);
PS3RT_API int  ps3rt_spu_mbox_read(int id, uint32_t* out);
PS3RT_API int  ps3rt_spu_mfc_dma(int id, uint32_t lsa, uint64_t ea, uint32_t size, uint32_t cmd);
PS3RT_API void     ps3rt_set_graphics_backend(int backend);
PS3RT_API int      ps3rt_get_graphics_backend(void);
PS3RT_API void     ps3rt_rsx_init(uint64_t fifo_addr, uint32_t fifo_size);
PS3RT_API void     ps3rt_rsx_set_put(uint32_t put);
PS3RT_API uint32_t ps3rt_rsx_get_get(void);
PS3RT_API void     ps3rt_rsx_flush(uint8_t* mem);
PS3RT_API void     ps3rt_rsx_flip(uint32_t buf_id);
PS3RT_API uint64_t ps3rt_rsx_flip_count(void);
PS3RT_API int      ps3rt_rsx_cell_gcm_syscall(PPUContext* c, uint64_t nid_or_num);
