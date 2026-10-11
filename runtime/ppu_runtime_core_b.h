// ppu_runtime_core_b.h - continuation of core (rd helpers, chunks, API decls)
// Appended after core_a
// NOTE: SPUContext is defined in core_a.h — do not redefine here

static inline float rd_f32(PPUContext& c, uint64_t addr) {
    uint32_t u = rd32(c, addr);
    float f; std::memcpy(&f, &u, 4); return f;
}
static inline void wr_f32(PPUContext& c, uint64_t addr, float v) {
    uint32_t u; std::memcpy(&u, &v, 4); wr32(c, addr, u);
}
static inline double rd_f64(PPUContext& c, uint64_t addr) {
    uint64_t u = rd64(c, addr);
    double d; std::memcpy(&d, &u, 8); return d;
}
static inline void wr_f64(PPUContext& c, uint64_t addr, double v) {
    uint64_t u; std::memcpy(&u, &v, 8); wr64(c, addr, u);
}

struct PPUChunk {
    uint64_t start;
    uint64_t end;
    bool (*fn)(PPUContext&);
};

static inline bool pc_in_chunks(uint64_t pc, const PPUChunk* chunks, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (pc >= chunks[i].start && pc < chunks[i].end) return true;
    return false;
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
PS3RT_API void     ps3rt_progress_hw(void);
PS3RT_API int      ps3rt_rsx_cell_gcm_syscall(PPUContext* c, uint64_t nid_or_num);
