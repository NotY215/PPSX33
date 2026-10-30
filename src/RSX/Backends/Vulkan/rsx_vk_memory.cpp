// WORK: Vulkan host memory — staging buffers for guest RGBA upload.
#include "rsx_vk_backend.h"
#include <cstdint>
#include <cstring>

extern "C" void ps3rt_log(const char* fmt, ...);

// These match the dynamic-loader path in rsx_vk_backend.cpp (same TU-visible helpers via weak link pattern).
// Staging create/destroy is also inlined in present_guest_rgba; this unit exposes reusable helpers.

struct VkMemoryRequirements { uint64_t size, alignment; uint32_t memoryTypeBits; };
struct VkMemoryAllocateInfo { int sType; const void* pNext; uint64_t allocationSize; uint32_t memoryTypeIndex; };
struct VkBufferCreateInfo {
    int sType; const void* pNext; uint32_t flags; uint64_t size; uint32_t usage; int sharingMode;
    uint32_t queueFamilyIndexCount; const uint32_t* pQueueFamilyIndices;
};
struct VkPhysicalDeviceMemoryProperties {
    uint32_t memoryTypeCount;
    struct { uint32_t propertyFlags; uint32_t heapIndex; } memoryTypes[32];
    uint32_t memoryHeapCount;
    struct { uint64_t size; uint32_t flags; } memoryHeaps[16];
};

enum { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
       VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 1, VK_SHARING_MODE_EXCLUSIVE = 0,
       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 2, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 4 };

// Backend owns the live device; helpers take explicit handles + proc addresses.
using PFN_GetMemProps = void (*)(void* phys, VkPhysicalDeviceMemoryProperties*);
using PFN_CreateBuffer = int (*)(void* dev, const VkBufferCreateInfo*, const void*, void**);
using PFN_DestroyBuffer = void (*)(void* dev, void* buf, const void*);
using PFN_GetBufReq = void (*)(void* dev, void* buf, VkMemoryRequirements*);
using PFN_AllocMem = int (*)(void* dev, const VkMemoryAllocateInfo*, const void*, void**);
using PFN_FreeMem = void (*)(void* dev, void* mem, const void*);
using PFN_BindBuf = int (*)(void* dev, void* buf, void* mem, uint64_t offset);
using PFN_Map = int (*)(void* dev, void* mem, uint64_t off, uint64_t size, uint32_t flags, void**);
using PFN_Unmap = void (*)(void* dev, void* mem);

uint32_t rsx_vk_mem_find_type(void* phys, PFN_GetMemProps get_props,
                              uint32_t type_bits, uint32_t props) {
    if (!phys || !get_props) return 0;
    VkPhysicalDeviceMemoryProperties mp{};
    get_props(phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((type_bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return 0;
}

int rsx_vk_mem_create_staging(
    void* device, void* phys,
    PFN_GetMemProps get_props,
    PFN_CreateBuffer create_buf, PFN_GetBufReq get_req,
    PFN_AllocMem alloc_mem, PFN_BindBuf bind_buf, PFN_Map map_mem,
    uint64_t size, const void* src_pixels,
    void** out_buf, void** out_mem)
{
    if (!device || !size || !out_buf || !out_mem) return 0;
    *out_buf = nullptr; *out_mem = nullptr;
    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    void* buf = nullptr;
    if (!create_buf || create_buf(device, &bci, nullptr, &buf) != 0) return 0;
    VkMemoryRequirements mr{};
    get_req(device, buf, &mr);
    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = rsx_vk_mem_find_type(phys, get_props, mr.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* mem = nullptr;
    if (!alloc_mem || alloc_mem(device, &mai, nullptr, &mem) != 0) {
        if (create_buf) { /* need destroy - caller cleans on fail path */ }
        return 0;
    }
    bind_buf(device, buf, mem, 0);
    if (src_pixels && map_mem) {
        void* mapped = nullptr;
        if (map_mem(device, mem, 0, size, 0, &mapped) == 0 && mapped) {
            std::memcpy(mapped, src_pixels, (size_t)size);
        }
    }
    *out_buf = buf;
    *out_mem = mem;
    return 1;
}

void rsx_vk_mem_destroy_staging(void* device, void* buf, void* mem,
    PFN_DestroyBuffer destroy_buf, PFN_FreeMem free_mem, PFN_Unmap unmap)
{
    (void)unmap;
    if (!device) return;
    if (buf && destroy_buf) destroy_buf(device, buf, nullptr);
    if (mem && free_mem) free_mem(device, mem, nullptr);
}
