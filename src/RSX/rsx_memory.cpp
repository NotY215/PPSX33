// WORK: RSX local + main memory map tracking.
#include "rsx_memory.h"
#include <mutex>
namespace ppsx33 { namespace rsx {
static uint64_t g_local_ea = 0;
static uint32_t g_local_size = 0;
static uint64_t g_main_ea = 0;
static uint32_t g_main_size = 0;
static std::mutex g_mem_mu;
void rsx_memory_init() {
    std::lock_guard<std::mutex> lk(g_mem_mu);
    g_local_ea = g_main_ea = 0;
    g_local_size = g_main_size = 0;
}
void rsx_memory_set_local(uint64_t ea, uint32_t size) {
    std::lock_guard<std::mutex> lk(g_mem_mu);
    g_local_ea = ea; g_local_size = size;
}
void rsx_memory_set_main(uint64_t ea, uint32_t size) {
    std::lock_guard<std::mutex> lk(g_mem_mu);
    g_main_ea = ea; g_main_size = size;
}
uint64_t rsx_memory_local_ea() { return g_local_ea; }
uint32_t rsx_memory_local_size() { return g_local_size; }
}}
