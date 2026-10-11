static inline float  rd_f32(PPUContext& c, uint64_t a) { uint32_t bits = rd32(c, a); float f; std::memcpy(&f, &bits, 4); return f; }
static inline double rd_f64(PPUContext& c, uint64_t a) { uint64_t bits = rd64(c, a); double d; std::memcpy(&d, &bits, 8); return d; }
static inline void wr_f32(PPUContext& c, uint64_t a, float f) { uint32_t bits; std::memcpy(&bits, &f, 4); wr32(c, a, bits); }
static inline void wr_f64(PPUContext& c, uint64_t a, double d) { uint64_t bits; std::memcpy(&bits, &d, 8); wr64(c, a, bits); }

static inline void xer_set_ca(PPUContext& c, bool v) { if (v) c.xer |= (1ull << 32); else c.xer &= ~(1ull << 32); }
static inline void xer_set_ov(PPUContext& c, bool v) { if (v) { c.xer |= (1ull << 33); c.xer |= (1ull << 34); } else c.xer &= ~(1ull << 33); }
static inline void xer_set_so(PPUContext& c, bool v) { if (v) c.xer |= (1ull << 34); else c.xer &= ~(1ull << 34); }
static inline bool xer_ca(const PPUContext& c) { return (c.xer >> 32) & 1; }
static inline bool xer_ov(const PPUContext& c) { return (c.xer >> 33) & 1; }
static inline bool xer_so(const PPUContext& c) { return (c.xer >> 34) & 1; }
static inline void xer_record_ca_add32(PPUContext& c, uint32_t a, uint32_t b, uint32_t res) { (void)b; xer_set_ca(c, res < a); }
static inline void xer_record_ca_add64(PPUContext& c, uint64_t a, uint64_t b, uint64_t res) { (void)b; xer_set_ca(c, res < a); }
static inline void xer_record_ca_sub32(PPUContext& c, uint32_t a, uint32_t b) { xer_set_ca(c, a >= b); }
static inline void xer_record_ca_sub64(PPUContext& c, uint64_t a, uint64_t b) { xer_set_ca(c, a >= b); }
static inline void xer_record_ov_add32(PPUContext& c, int32_t a, int32_t b, int32_t res) { xer_set_ov(c, ((a ^ res) & (b ^ res)) < 0); }
static inline void xer_record_ov_sub32(PPUContext& c, int32_t a, int32_t b, int32_t res) { xer_set_ov(c, ((a ^ b) & (a ^ res)) < 0); }

static inline void set_cr_signed(PPUContext& c, int f, int64_t a, int64_t b){
    uint8_t bits = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
    if (f == 0 && xer_so(c)) bits |= 1; c.cr[f] = bits;
}
static inline void set_cr_unsigned(PPUContext& c, int f, uint64_t a, uint64_t b){
    uint8_t bits = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
    if (f == 0 && xer_so(c)) bits |= 1; c.cr[f] = bits;
}
static inline void set_cr_fp(PPUContext& c, int f, double a, double b){
    if (std::isnan(a) || std::isnan(b)) { c.cr[f] = 1; return; }
    c.cr[f] = (uint8_t)((a < b ? 8 : 0) | (a > b ? 4 : 0) | (a == b ? 2 : 0));
}

enum {
    FPSCR_FX = 0x80000000u, FPSCR_FEX = 0x40000000u, FPSCR_VX = 0x20000000u,
    FPSCR_OX = 0x10000000u, FPSCR_UX = 0x08000000u, FPSCR_ZX = 0x04000000u,
    FPSCR_XX = 0x02000000u, FPSCR_VXSNAN = 0x01000000u, FPSCR_VXISI = 0x00800000u,
    FPSCR_VXIDI = 0x00400000u, FPSCR_VXZDZ = 0x00200000u, FPSCR_VXIMZ = 0x00100000u,
    FPSCR_VXVC = 0x00080000u, FPSCR_FR = 0x00040000u, FPSCR_FI = 0x00020000u,
    FPSCR_FPRF_MASK = 0x0001F000u, FPSCR_C = 0x00010000u,
};
static inline void fpscr_set_fprf(PPUContext& c, double v) {
    c.fpscr &= ~FPSCR_FPRF_MASK;
    if (std::isnan(v)) c.fpscr |= 0x00011000u;
    else if (v == 0.0) c.fpscr |= (std::signbit(v) ? 0x00012000u : 0x00002000u);
    else if (std::isinf(v)) c.fpscr |= (v < 0 ? 0x00009000u : 0x00005000u);
    else c.fpscr |= (v < 0 ? 0x00008000u : 0x00004000u);
}
static inline void fpscr_set_exception(PPUContext& c, uint32_t flag) {
    c.fpscr |= flag | FPSCR_FX;
    if (flag & (FPSCR_VX|FPSCR_OX|FPSCR_UX|FPSCR_ZX|FPSCR_XX)) c.fpscr |= FPSCR_FEX;
}
static inline void fpscr_record_fcmp(PPUContext& c, int crf, double a, double b) {
    if (std::isnan(a) || std::isnan(b)) {
        c.cr[crf] = 1; fpscr_set_exception(c, FPSCR_VX | FPSCR_VXSNAN | FPSCR_VXVC); c.fpscr |= FPSCR_C; return;
    }
    set_cr_fp(c, crf, a, b); c.fpscr &= ~FPSCR_C;
}
static inline void fpscr_record_arith(PPUContext& c, double result) {
    fpscr_set_fprf(c, result);
    if (std::isnan(result)) fpscr_set_exception(c, FPSCR_VX);
    else if (std::isinf(result)) fpscr_set_exception(c, FPSCR_OX);
}
static inline bool bc_taken(PPUContext& c, unsigned bo, unsigned bi){
    bool ctr_ok = true, cond_ok = true;
    if (!(bo & 0x04)) { c.ctr -= 1; ctr_ok = ((c.ctr != 0) != ((bo & 0x02) != 0)); }
    if (!(bo & 0x10)) {
        bool bit = (c.cr[bi >> 2] >> (3 - (bi & 3))) & 1;
        cond_ok = (bit == ((bo & 0x08) != 0));
    }
    return ctr_ok && cond_ok;
}

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

struct PPUChunk { uint64_t start, end; bool (*fn)(PPUContext&); };

static inline bool pc_in_chunks(uint64_t pc, const PPUChunk* chunks, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (pc >= chunks[i].start && pc < chunks[i].end) return true;
    return false;
}
