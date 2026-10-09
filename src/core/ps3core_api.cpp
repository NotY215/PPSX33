#define PS3CORE_BUILD 1
#include "ps3core.h"
#include "project.h"
#include <cstring>
#include <string>

static void put(char* dst, int n, const std::string& s) {
    if (!dst || n <= 0) return;
    std::strncpy(dst, s.c_str(), (size_t)n - 1);
    dst[n - 1] = 0;
}

extern "C" {

PS3CORE_API int ps3_core_version(void) { return 1; }

PS3CORE_API int ps3_create_project(const char* root, const char* game, const char* elf,
                                   char* pd, int pdn, char* err, int errn) {
    std::string dir, e;
    if (!ps3::create_project(root, game, elf, dir, e)) { put(err, errn, e); return 1; }
    put(pd, pdn, dir);
    return 0;
}

PS3CORE_API int ps3_lift_project(const char* pd, char* log, int logn) {
    std::string l, e;
    bool ok = ps3::lift_project(pd, l, e);
    put(log, logn, ok ? l : e);
    return ok ? 0 : 1;
}

PS3CORE_API int ps3_build_project(const char* pd, const char* rt, const char* comp, char* log, int logn) {
    std::string l, e;
    bool ok = ps3::build_project(pd, rt, comp, l, e);
    put(log, logn, ok ? l : (l + "\n" + e));
    return ok ? 0 : 1;
}

}
