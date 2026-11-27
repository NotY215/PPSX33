// WORK: VFS maps guest PS3 paths onto the game USRDIR host root.
#include "vfs.h"
#include <cstring>
#include <string>

namespace ppsx33::fs {
static std::string g_root;

void set_root_overlay(const std::string& r) { g_root = r; }

static void replace_sep(std::string& s) {
    for (char& c : s)
        if (c == '\\') c = '/';
}

std::string resolve_guest_path(const std::string& guest_in) {
    std::string g = guest_in;
    replace_sep(g);
    while (!g.empty() && (g[0] == ' ')) g.erase(g.begin());

    const char* prefixes[] = {
        "/dev_hdd0/game/", "/dev_hdd0/",
        "/app_home/",
        "/dev_bdvd/PS3_GAME/USRDIR/", "/dev_bdvd/PS3_GAME/", "/dev_bdvd/",
        "/USRDIR/", "/PS3_GAME/USRDIR/", "/PS3_GAME/",
        nullptr
    };
    for (int i = 0; prefixes[i]; ++i) {
        size_t n = std::strlen(prefixes[i]);
        if (g.size() >= n) {
            bool eq = true;
            for (size_t k = 0; k < n; ++k) {
                char a = g[k], b = prefixes[i][k];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { eq = false; break; }
            }
            if (eq) {
                g = g.substr(n);
                break;
            }
        }
    }
    while (!g.empty() && g[0] == '/') g.erase(g.begin());
    if (g_root.empty()) return g;
    if (g.empty()) return g_root;
#ifdef _WIN32
    char sep = '\\';
#else
    char sep = '/';
#endif
    std::string out = g_root;
    if (!out.empty() && out.back() != '/' && out.back() != '\\') out.push_back(sep);
    for (char c : g) out.push_back(c == '/' ? sep : c);
    return out;
}
}
