// WORK: Vulkan command recording — image barriers and buffer→image copies.
#include "rsx_vk_backend.h"
#include <cstdint>

enum {
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 45,
    VK_IMAGE_ASPECT_COLOR_BIT = 1,
    VK_IMAGE_LAYOUT_UNDEFINED = 0,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 1,
    VK_PIPELINE_STAGE_TRANSFER_BIT = 0x1000,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x200,
    VK_ACCESS_TRANSFER_WRITE_BIT = 0x1000,
    VK_ACCESS_MEMORY_READ_BIT = 0x8000
};

struct VkImageMemoryBarrier {
    int sType; const void* pNext; uint32_t srcAccessMask, dstAccessMask;
    int oldLayout, newLayout; uint32_t srcQueueFamilyIndex, dstQueueFamilyIndex;
    void* image;
    struct { uint32_t aspectMask; uint32_t baseMipLevel, levelCount, baseArrayLayer, layerCount; } subresourceRange;
};

struct VkBufferImageCopy {
    uint64_t bufferOffset; uint32_t bufferRowLength, bufferImageHeight;
    struct { uint32_t aspectMask; uint32_t mipLevel, baseArrayLayer, layerCount; } imageSubresource;
    struct { int32_t x, y, z; } imageOffset;
    struct { uint32_t width, height, depth; } imageExtent;
};

using PFN_CmdBarrier = void (*)(void* cmd, uint32_t srcStage, uint32_t dstStage, uint32_t depFlags,
    uint32_t memCount, const void*, uint32_t bufCount, const void*,
    uint32_t imgCount, const VkImageMemoryBarrier*);
using PFN_CmdCopyBufImg = void (*)(void* cmd, void* buf, void* img, int layout,
    uint32_t regionCount, const VkBufferImageCopy*);

void rsx_vk_cmd_barrier_to_transfer_dst(void* cmd, void* image, PFN_CmdBarrier barrier) {
    if (!cmd || !image || !barrier) return;
    VkImageMemoryBarrier b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = 0xFFFFFFFFu;
    b.dstQueueFamilyIndex = 0xFFFFFFFFu;
    b.image = image;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;
    barrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &b);
}

void rsx_vk_cmd_barrier_to_present(void* cmd, void* image, PFN_CmdBarrier barrier) {
    if (!cmd || !image || !barrier) return;
    VkImageMemoryBarrier b{};
    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    b.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    b.srcQueueFamilyIndex = 0xFFFFFFFFu;
    b.dstQueueFamilyIndex = 0xFFFFFFFFu;
    b.image = image;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;
    barrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &b);
}

void rsx_vk_cmd_copy_rgba_to_image(void* cmd, void* staging_buf, void* image,
    uint32_t w, uint32_t h, uint32_t row_pixels, PFN_CmdCopyBufImg copy)
{
    if (!cmd || !staging_buf || !image || !copy || !w || !h) return;
    VkBufferImageCopy region{};
    region.bufferRowLength = row_pixels ? row_pixels : w;
    region.bufferImageHeight = h;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = w;
    region.imageExtent.height = h;
    region.imageExtent.depth = 1;
    copy(cmd, staging_buf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
