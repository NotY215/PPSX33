#include "import_stub_map.h"

#define IMPORT_BIND_CAP 512
typedef struct { uint64_t ea; uint32_t nid; } bind_t;
static bind_t g_binds[IMPORT_BIND_CAP];
static int g_n;

void ps3rt_import_bind(uint64_t ea, uint32_t nid) {
    int i;
    if (!ea || !nid) return;
    ea &= ~(uint64_t)3;
    for (i = 0; i < g_n; ++i) {
        if (g_binds[i].ea == ea) { g_binds[i].nid = nid; return; }
    }
    if (g_n < IMPORT_BIND_CAP) {
        g_binds[g_n].ea = ea;
        g_binds[g_n].nid = nid;
        ++g_n;
    }
}

uint32_t ps3rt_import_lookup(uint64_t ea) {
    int i;
    uint64_t ea4;
    ea &= ~(uint64_t)3;
    for (i = 0; i < g_n; ++i)
        if (g_binds[i].ea == ea) return g_binds[i].nid;
    ea4 = (ea > 4) ? ((ea - 4) & ~(uint64_t)3) : 0;
    for (i = 0; i < g_n; ++i)
        if (g_binds[i].ea == ea4) return g_binds[i].nid;
    return 0;
}

void ps3rt_import_clear(void) { g_n = 0; }
int  ps3rt_import_bind_count(void) { return g_n; }
