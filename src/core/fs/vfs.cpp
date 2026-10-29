// WORK: VFS path mapping.
#include "vfs.h"
namespace ppsx33::fs {
static std::string g_root;
void set_root_overlay(const std::string& r){g_root=r;}
std::string resolve_guest_path(const std::string& g){return g_root.empty()?g:(g_root+"/"+g);}
}
