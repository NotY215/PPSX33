// WORK: Vulkan render-pass / framebuffer pipeline objects for clear+present.
#include "rsx_vk_backend.h"
#include <cstdint>

enum {
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO = 38,
    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO = 37,
    VK_ATTACHMENT_LOAD_OP_CLEAR = 1,
    VK_ATTACHMENT_STORE_OP_STORE = 0,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE = 2,
    VK_ATTACHMENT_STORE_OP_DONT_CARE = 1,
    VK_SAMPLE_COUNT_1_BIT = 1,
    VK_IMAGE_LAYOUT_UNDEFINED = 0,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    VK_PIPELINE_BIND_POINT_GRAPHICS = 0,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x400,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x100
};

struct VkAttachmentDescription {
    uint32_t flags; int format; int samples; int loadOp, storeOp, stencilLoadOp, stencilStoreOp;
    int initialLayout, finalLayout;
};
struct VkAttachmentReference { uint32_t attachment; int layout; };
struct VkSubpassDescription {
    uint32_t flags; int pipelineBindPoint;
    uint32_t inputAttachmentCount; const VkAttachmentReference* pInputAttachments;
    uint32_t colorAttachmentCount; const VkAttachmentReference* pColorAttachments;
    const VkAttachmentReference* pResolveAttachments;
    const VkAttachmentReference* pDepthStencilAttachment;
    uint32_t preserveAttachmentCount; const uint32_t* pPreserveAttachments;
};
struct VkSubpassDependency {
    uint32_t srcSubpass, dstSubpass; uint32_t srcStageMask, dstStageMask, srcAccessMask, dstAccessMask;
    uint32_t dependencyFlags;
};
struct VkRenderPassCreateInfo {
    int sType; const void* pNext; uint32_t flags;
    uint32_t attachmentCount; const VkAttachmentDescription* pAttachments;
    uint32_t subpassCount; const VkSubpassDescription* pSubpasses;
    uint32_t dependencyCount; const VkSubpassDependency* pDependencies;
};
struct VkFramebufferCreateInfo {
    int sType; const void* pNext; uint32_t flags; void* renderPass;
    uint32_t attachmentCount; void* const* pAttachments;
    uint32_t width, height, layers;
};

using PFN_CreateRP = int (*)(void* dev, const VkRenderPassCreateInfo*, const void*, void**);
using PFN_DestroyRP = void (*)(void* dev, void* rp, const void*);
using PFN_CreateFB = int (*)(void* dev, const VkFramebufferCreateInfo*, const void*, void**);
using PFN_DestroyFB = void (*)(void* dev, void* fb, const void*);

int rsx_vk_pipe_create_render_pass(void* device, int format, PFN_CreateRP create_rp, void** out_rp) {
    if (!device || !create_rp || !out_rp) return 0;
    VkAttachmentDescription att{};
    att.format = format;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference ref{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &ref;
    VkSubpassDependency dep{};
    dep.srcSubpass = 0xFFFFFFFFu;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &att;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sub;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    return create_rp(device, &rpci, nullptr, out_rp) == 0;
}

int rsx_vk_pipe_create_framebuffer(void* device, void* render_pass, void* view,
    uint32_t w, uint32_t h, PFN_CreateFB create_fb, void** out_fb)
{
    if (!device || !render_pass || !view || !create_fb || !out_fb) return 0;
    void* atts[] = { view };
    VkFramebufferCreateInfo fbci{};
    fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.renderPass = render_pass;
    fbci.attachmentCount = 1;
    fbci.pAttachments = atts;
    fbci.width = w;
    fbci.height = h;
    fbci.layers = 1;
    return create_fb(device, &fbci, nullptr, out_fb) == 0;
}

void rsx_vk_pipe_destroy_framebuffer(void* device, void* fb, PFN_DestroyFB destroy_fb) {
    if (device && fb && destroy_fb) destroy_fb(device, fb, nullptr);
}

void rsx_vk_pipe_destroy_render_pass(void* device, void* rp, PFN_DestroyRP destroy_rp) {
    if (device && rp && destroy_rp) destroy_rp(device, rp, nullptr);
}
