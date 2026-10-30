// WORK: Vulkan 1.2 host backend — dynamic vulkan-1.dll, swapchain clear/present.
#include "rsx_vk_backend.h"
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <vector>
#ifndef UINT64_MAX
#define UINT64_MAX 0xffffffffffffffffull
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

extern "C" void ps3rt_log(const char* fmt, ...);

// Minimal Vulkan types (avoid requiring SDK headers at compile time)
using VkFlags = uint32_t;
using VkBool32 = uint32_t;
using VkDeviceSize = uint64_t;
using VkResult = int32_t;

enum {
    VK_SUCCESS = 0,
    VK_TIMEOUT = 2,
    VK_NOT_READY = 1,
    VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
    VK_SUBOPTIMAL_KHR = 1000001003,
    VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
    VK_STRUCTURE_TYPE_SUBMIT_INFO = 4,
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40,
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42,
    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO = 43,
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15,
    VK_STRUCTURE_TYPE_MEMORY_BARRIER = 46,
    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER = 47,
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 45,
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8,
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO = 9,
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR = 1000009000,
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000,
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001,
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39,
    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO = 37,
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO = 38,
    VK_QUEUE_GRAPHICS_BIT = 1,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 1,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 2,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 4,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x10,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x02,
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x01,
    VK_IMAGE_ASPECT_COLOR_BIT = 1,
    VK_FORMAT_B8G8R8A8_UNORM = 44,
    VK_FORMAT_R8G8B8A8_UNORM = 37,
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR = 0,
    VK_PRESENT_MODE_FIFO_KHR = 2,
    VK_PRESENT_MODE_MAILBOX_KHR = 1,
    VK_SHARING_MODE_EXCLUSIVE = 0,
    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR = 1,
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR = 1,
    VK_IMAGE_VIEW_TYPE_2D = 1,
    VK_COMPONENT_SWIZZLE_IDENTITY = 0,
    VK_IMAGE_LAYOUT_UNDEFINED = 0,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 1,
    VK_PIPELINE_STAGE_TRANSFER_BIT = 0x1000,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x400,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x200,
    VK_ACCESS_TRANSFER_WRITE_BIT = 0x1000,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x100,
    VK_ACCESS_MEMORY_READ_BIT = 0x8000,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0,
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 1,
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 2,
    VK_FENCE_CREATE_SIGNALED_BIT = 1,
    VK_ATTACHMENT_LOAD_OP_CLEAR = 1,
    VK_ATTACHMENT_STORE_OP_STORE = 0,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE = 2,
    VK_ATTACHMENT_STORE_OP_DONT_CARE = 1,
    VK_SAMPLE_COUNT_1_BIT = 1,
    VK_IMAGE_TILING_OPTIMAL = 0,
    VK_IMAGE_TILING_LINEAR = 1,
    VK_IMAGE_TYPE_2D = 1,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 1,
    VK_PIPELINE_BIND_POINT_GRAPHICS = 0,
    VK_SUBPASS_CONTENTS_INLINE = 0,
    VK_TRUE = 1,
    VK_FALSE = 0,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
};

struct VkExtent2D { uint32_t width, height; };
struct VkOffset2D { int32_t x, y; };
struct VkRect2D { VkOffset2D offset; VkExtent2D extent; };
struct VkExtent3D { uint32_t width, height, depth; };
struct VkApplicationInfo {
    int sType; const void* pNext; const char* pApplicationName; uint32_t applicationVersion;
    const char* pEngineName; uint32_t engineVersion; uint32_t apiVersion;
};
struct VkInstanceCreateInfo {
    int sType; const void* pNext; VkFlags flags; const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount; const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount; const char* const* ppEnabledExtensionNames;
};
struct VkWin32SurfaceCreateInfoKHR {
    int sType; const void* pNext; VkFlags flags; void* hinstance; void* hwnd;
};
struct VkQueueFamilyProperties {
    VkFlags queueFlags; uint32_t queueCount; uint32_t timestampValidBits; VkExtent3D minImageTransferGranularity;
};
struct VkDeviceQueueCreateInfo {
    int sType; const void* pNext; VkFlags flags; uint32_t queueFamilyIndex; uint32_t queueCount; const float* pQueuePriorities;
};
struct VkDeviceCreateInfo {
    int sType; const void* pNext; VkFlags flags;
    uint32_t queueCreateInfoCount; const VkDeviceQueueCreateInfo* pQueueCreateInfos;
    uint32_t enabledLayerCount; const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount; const char* const* ppEnabledExtensionNames;
    const void* pEnabledFeatures;
};
struct VkSurfaceCapabilitiesKHR {
    uint32_t minImageCount, maxImageCount; VkExtent2D currentExtent, minImageExtent, maxImageExtent;
    uint32_t maxImageArrayLayers; VkFlags supportedTransforms; VkFlags currentTransform;
    VkFlags supportedCompositeAlpha; VkFlags supportedUsageFlags;
};
struct VkSurfaceFormatKHR { int format; int colorSpace; };
struct VkSwapchainCreateInfoKHR {
    int sType; const void* pNext; VkFlags flags; void* surface; uint32_t minImageCount;
    int imageFormat; int imageColorSpace; VkExtent2D imageExtent; uint32_t imageArrayLayers;
    VkFlags imageUsage; int imageSharingMode; uint32_t queueFamilyIndexCount; const uint32_t* pQueueFamilyIndices;
    VkFlags preTransform; VkFlags compositeAlpha; int presentMode; VkBool32 clipped; void* oldSwapchain;
};
struct VkImageViewCreateInfo {
    int sType; const void* pNext; VkFlags flags; void* image; int viewType; int format;
    struct { int r,g,b,a; } components;
    struct { VkFlags aspectMask; uint32_t baseMipLevel, levelCount, baseArrayLayer, layerCount; } subresourceRange;
};
struct VkAttachmentDescription {
    VkFlags flags; int format; int samples; int loadOp, storeOp, stencilLoadOp, stencilStoreOp;
    int initialLayout, finalLayout;
};
struct VkAttachmentReference { uint32_t attachment; int layout; };
struct VkSubpassDescription {
    VkFlags flags; int pipelineBindPoint; uint32_t inputAttachmentCount; const VkAttachmentReference* pInputAttachments;
    uint32_t colorAttachmentCount; const VkAttachmentReference* pColorAttachments;
    const VkAttachmentReference* pResolveAttachments; const VkAttachmentReference* pDepthStencilAttachment;
    uint32_t preserveAttachmentCount; const uint32_t* pPreserveAttachments;
};
struct VkSubpassDependency {
    uint32_t srcSubpass, dstSubpass; VkFlags srcStageMask, dstStageMask, srcAccessMask, dstAccessMask; VkFlags dependencyFlags;
};
struct VkRenderPassCreateInfo {
    int sType; const void* pNext; VkFlags flags;
    uint32_t attachmentCount; const VkAttachmentDescription* pAttachments;
    uint32_t subpassCount; const VkSubpassDescription* pSubpasses;
    uint32_t dependencyCount; const VkSubpassDependency* pDependencies;
};
struct VkFramebufferCreateInfo {
    int sType; const void* pNext; VkFlags flags; void* renderPass;
    uint32_t attachmentCount; void* const* pAttachments; uint32_t width, height, layers;
};
struct VkCommandPoolCreateInfo {
    int sType; const void* pNext; VkFlags flags; uint32_t queueFamilyIndex;
};
struct VkCommandBufferAllocateInfo {
    int sType; const void* pNext; void* commandPool; int level; uint32_t commandBufferCount;
};
struct VkCommandBufferBeginInfo {
    int sType; const void* pNext; VkFlags flags; const void* pInheritanceInfo;
};
struct VkClearColorValue { float float32[4]; };
struct VkClearValue { VkClearColorValue color; };
struct VkRenderPassBeginInfo {
    int sType; const void* pNext; void* renderPass; void* framebuffer; VkRect2D renderArea;
    uint32_t clearValueCount; const VkClearValue* pClearValues;
};
struct VkImageMemoryBarrier {
    int sType; const void* pNext; VkFlags srcAccessMask, dstAccessMask;
    int oldLayout, newLayout; uint32_t srcQueueFamilyIndex, dstQueueFamilyIndex;
    void* image;
    struct { VkFlags aspectMask; uint32_t baseMipLevel, levelCount, baseArrayLayer, layerCount; } subresourceRange;
};
struct VkSubmitInfo {
    int sType; const void* pNext;
    uint32_t waitSemaphoreCount; void* const* pWaitSemaphores; const VkFlags* pWaitDstStageMask;
    uint32_t commandBufferCount; void* const* pCommandBuffers;
    uint32_t signalSemaphoreCount; void* const* pSignalSemaphores;
};
struct VkPresentInfoKHR {
    int sType; const void* pNext;
    uint32_t waitSemaphoreCount; void* const* pWaitSemaphores;
    uint32_t swapchainCount; void* const* pSwapchains; const uint32_t* pImageIndices; VkResult* pResults;
};
struct VkSemaphoreCreateInfo { int sType; const void* pNext; VkFlags flags; };
struct VkFenceCreateInfo { int sType; const void* pNext; VkFlags flags; };
struct VkMemoryRequirements { VkDeviceSize size, alignment; uint32_t memoryTypeBits; };
struct VkMemoryAllocateInfo { int sType; const void* pNext; VkDeviceSize allocationSize; uint32_t memoryTypeIndex; };
struct VkBufferCreateInfo {
    int sType; const void* pNext; VkFlags flags; VkDeviceSize size; VkFlags usage; int sharingMode;
    uint32_t queueFamilyIndexCount; const uint32_t* pQueueFamilyIndices;
};
struct VkImageCreateInfo {
    int sType; const void* pNext; VkFlags flags; int imageType; int format; VkExtent3D extent;
    uint32_t mipLevels, arrayLayers; int samples; int tiling; VkFlags usage; int sharingMode;
    uint32_t queueFamilyIndexCount; const uint32_t* pQueueFamilyIndices; int initialLayout;
};
struct VkBufferImageCopy {
    VkDeviceSize bufferOffset; uint32_t bufferRowLength, bufferImageHeight;
    struct { VkFlags aspectMask; uint32_t mipLevel, baseArrayLayer, layerCount; } imageSubresource;
    struct { int32_t x,y,z; } imageOffset; VkExtent3D imageExtent;
};
struct VkPhysicalDeviceMemoryProperties {
    uint32_t memoryTypeCount;
    struct { uint32_t propertyFlags; uint32_t heapIndex; } memoryTypes[32];
    uint32_t memoryHeapCount;
    struct { VkDeviceSize size; uint32_t flags; } memoryHeaps[16];
};
struct VkPhysicalDeviceProperties {
    uint32_t apiVersion, driverVersion, vendorID, deviceID; int deviceType;
    char deviceName[256]; uint8_t pipelineCacheUUID[16];
    uint8_t limits_pad[512];
};

using PFN_vkVoid = void (*)(void);
using PFN_vkGetInstanceProcAddr = void* (*)(void* instance, const char* name);
using PFN_vkCreateInstance = VkResult (*)(const VkInstanceCreateInfo*, const void*, void**);
using PFN_vkDestroyInstance = void (*)(void*, const void*);
using PFN_vkEnumeratePhysicalDevices = VkResult (*)(void*, uint32_t*, void**);
using PFN_vkGetPhysicalDeviceQueueFamilyProperties = void (*)(void*, uint32_t*, VkQueueFamilyProperties*);
using PFN_vkGetPhysicalDeviceSurfaceSupportKHR = VkResult (*)(void*, uint32_t, void*, VkBool32*);
using PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = VkResult (*)(void*, void*, VkSurfaceCapabilitiesKHR*);
using PFN_vkGetPhysicalDeviceSurfaceFormatsKHR = VkResult (*)(void*, void*, uint32_t*, VkSurfaceFormatKHR*);
using PFN_vkGetPhysicalDeviceSurfacePresentModesKHR = VkResult (*)(void*, void*, uint32_t*, int*);
using PFN_vkGetPhysicalDeviceProperties = void (*)(void*, VkPhysicalDeviceProperties*);
using PFN_vkGetPhysicalDeviceMemoryProperties = void (*)(void*, VkPhysicalDeviceMemoryProperties*);
using PFN_vkCreateDevice = VkResult (*)(void*, const VkDeviceCreateInfo*, const void*, void**);
using PFN_vkDestroyDevice = void (*)(void*, const void*);
using PFN_vkGetDeviceQueue = void (*)(void*, uint32_t, uint32_t, void**);
using PFN_vkCreateSwapchainKHR = VkResult (*)(void*, const VkSwapchainCreateInfoKHR*, const void*, void**);
using PFN_vkDestroySwapchainKHR = void (*)(void*, void*, const void*);
using PFN_vkGetSwapchainImagesKHR = VkResult (*)(void*, void*, uint32_t*, void**);
using PFN_vkCreateImageView = VkResult (*)(void*, const VkImageViewCreateInfo*, const void*, void**);
using PFN_vkDestroyImageView = void (*)(void*, void*, const void*);
using PFN_vkCreateRenderPass = VkResult (*)(void*, const VkRenderPassCreateInfo*, const void*, void**);
using PFN_vkDestroyRenderPass = void (*)(void*, void*, const void*);
using PFN_vkCreateFramebuffer = VkResult (*)(void*, const VkFramebufferCreateInfo*, const void*, void**);
using PFN_vkDestroyFramebuffer = void (*)(void*, void*, const void*);
using PFN_vkCreateCommandPool = VkResult (*)(void*, const VkCommandPoolCreateInfo*, const void*, void**);
using PFN_vkDestroyCommandPool = void (*)(void*, void*, const void*);
using PFN_vkAllocateCommandBuffers = VkResult (*)(void*, const VkCommandBufferAllocateInfo*, void**);
using PFN_vkBeginCommandBuffer = VkResult (*)(void*, const VkCommandBufferBeginInfo*);
using PFN_vkEndCommandBuffer = VkResult (*)(void*);
using PFN_vkCmdBeginRenderPass = void (*)(void*, const VkRenderPassBeginInfo*, int);
using PFN_vkCmdEndRenderPass = void (*)(void*);
using PFN_vkCmdPipelineBarrier = void (*)(void*, VkFlags, VkFlags, VkFlags, uint32_t, const void*, uint32_t, const void*, uint32_t, const VkImageMemoryBarrier*);
using PFN_vkCmdClearColorImage = void (*)(void*, void*, int, const VkClearColorValue*, uint32_t, const void*);
using PFN_vkCreateSemaphore = VkResult (*)(void*, const VkSemaphoreCreateInfo*, const void*, void**);
using PFN_vkDestroySemaphore = void (*)(void*, void*, const void*);
using PFN_vkCreateFence = VkResult (*)(void*, const VkFenceCreateInfo*, const void*, void**);
using PFN_vkDestroyFence = void (*)(void*, void*, const void*);
using PFN_vkWaitForFences = VkResult (*)(void*, uint32_t, void* const*, VkBool32, uint64_t);
using PFN_vkResetFences = VkResult (*)(void*, uint32_t, void* const*);
using PFN_vkQueueSubmit = VkResult (*)(void*, uint32_t, const VkSubmitInfo*, void*);
using PFN_vkQueuePresentKHR = VkResult (*)(void*, const VkPresentInfoKHR*);
using PFN_vkAcquireNextImageKHR = VkResult (*)(void*, void*, uint64_t, void*, void*, uint32_t*);
using PFN_vkDeviceWaitIdle = VkResult (*)(void*);
using PFN_vkCreateWin32SurfaceKHR = VkResult (*)(void*, const VkWin32SurfaceCreateInfoKHR*, const void*, void**);
using PFN_vkDestroySurfaceKHR = void (*)(void*, void*, const void*);
using PFN_vkCreateBuffer = VkResult (*)(void*, const VkBufferCreateInfo*, const void*, void**);
using PFN_vkDestroyBuffer = void (*)(void*, void*, const void*);
using PFN_vkGetBufferMemoryRequirements = void (*)(void*, void*, VkMemoryRequirements*);
using PFN_vkAllocateMemory = VkResult (*)(void*, const VkMemoryAllocateInfo*, const void*, void**);
using PFN_vkFreeMemory = void (*)(void*, void*, const void*);
using PFN_vkBindBufferMemory = VkResult (*)(void*, void*, void*, VkDeviceSize);
using PFN_vkMapMemory = VkResult (*)(void*, void*, VkDeviceSize, VkDeviceSize, VkFlags, void**);
using PFN_vkUnmapMemory = void (*)(void*, void*);
using PFN_vkCmdCopyBufferToImage = void (*)(void*, void*, void*, int, uint32_t, const VkBufferImageCopy*);
using PFN_vkResetCommandBuffer = VkResult (*)(void*, VkFlags);

static struct {
    HMODULE lib = nullptr;
    void* instance = nullptr;
    void* surface = nullptr;
    void* phys = nullptr;
    void* device = nullptr;
    void* queue = nullptr;
    void* swapchain = nullptr;
    void* render_pass = nullptr;
    void* cmd_pool = nullptr;
    void* cmd = nullptr;
    void* image_available = nullptr;
    void* render_finished = nullptr;
    void* in_flight = nullptr;
    uint32_t qfamily = 0;
    uint32_t width = 0, height = 0;
    int format = VK_FORMAT_B8G8R8A8_UNORM;
    std::vector<void*> images;
    std::vector<void*> views;
    std::vector<void*> framebuffers;
    int ready = 0;

    PFN_vkGetInstanceProcAddr GetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance CreateInstance = nullptr;
    PFN_vkDestroyInstance DestroyInstance = nullptr;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR = nullptr;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties = nullptr;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties = nullptr;
    PFN_vkCreateDevice CreateDevice = nullptr;
    PFN_vkDestroyDevice DestroyDevice = nullptr;
    PFN_vkGetDeviceQueue GetDeviceQueue = nullptr;
    PFN_vkCreateSwapchainKHR CreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR DestroySwapchainKHR = nullptr;
    PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;
    PFN_vkCreateImageView CreateImageView = nullptr;
    PFN_vkDestroyImageView DestroyImageView = nullptr;
    PFN_vkCreateRenderPass CreateRenderPass = nullptr;
    PFN_vkDestroyRenderPass DestroyRenderPass = nullptr;
    PFN_vkCreateFramebuffer CreateFramebuffer = nullptr;
    PFN_vkDestroyFramebuffer DestroyFramebuffer = nullptr;
    PFN_vkCreateCommandPool CreateCommandPool = nullptr;
    PFN_vkDestroyCommandPool DestroyCommandPool = nullptr;
    PFN_vkAllocateCommandBuffers AllocateCommandBuffers = nullptr;
    PFN_vkBeginCommandBuffer BeginCommandBuffer = nullptr;
    PFN_vkEndCommandBuffer EndCommandBuffer = nullptr;
    PFN_vkCmdBeginRenderPass CmdBeginRenderPass = nullptr;
    PFN_vkCmdEndRenderPass CmdEndRenderPass = nullptr;
    PFN_vkCmdPipelineBarrier CmdPipelineBarrier = nullptr;
    PFN_vkCmdClearColorImage CmdClearColorImage = nullptr;
    PFN_vkCreateSemaphore CreateSemaphore = nullptr;
    PFN_vkDestroySemaphore DestroySemaphore = nullptr;
    PFN_vkCreateFence CreateFence = nullptr;
    PFN_vkDestroyFence DestroyFence = nullptr;
    PFN_vkWaitForFences WaitForFences = nullptr;
    PFN_vkResetFences ResetFences = nullptr;
    PFN_vkQueueSubmit QueueSubmit = nullptr;
    PFN_vkQueuePresentKHR QueuePresentKHR = nullptr;
    PFN_vkAcquireNextImageKHR AcquireNextImageKHR = nullptr;
    PFN_vkDeviceWaitIdle DeviceWaitIdle = nullptr;
    PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR = nullptr;
    PFN_vkDestroySurfaceKHR DestroySurfaceKHR = nullptr;
    PFN_vkCreateBuffer CreateBuffer = nullptr;
    PFN_vkDestroyBuffer DestroyBuffer = nullptr;
    PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements = nullptr;
    PFN_vkAllocateMemory AllocateMemory = nullptr;
    PFN_vkFreeMemory FreeMemory = nullptr;
    PFN_vkBindBufferMemory BindBufferMemory = nullptr;
    PFN_vkMapMemory MapMemory = nullptr;
    PFN_vkUnmapMemory UnmapMemory = nullptr;
    PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage = nullptr;
    PFN_vkResetCommandBuffer ResetCommandBuffer = nullptr;
} g_vk;

static std::mutex g_vk_mu;

static void* vk_ipa(void* inst, const char* name) {
    return g_vk.GetInstanceProcAddr ? g_vk.GetInstanceProcAddr(inst, name) : nullptr;
}
static void* vk_dpa(void* dev, const char* name) {
    auto f = (void*(*)(void*, const char*))vk_ipa(g_vk.instance, "vkGetDeviceProcAddr");
    return f ? f(dev, name) : vk_ipa(nullptr, name);
}

static int load_vulkan_lib() {
    if (g_vk.lib) return 1;
#if defined(_WIN32)
    g_vk.lib = LoadLibraryA("vulkan-1.dll");
    if (!g_vk.lib) {
        ps3rt_log("[rsx] Vulkan: vulkan-1.dll not found\n");
        return 0;
    }
    g_vk.GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(g_vk.lib, "vkGetInstanceProcAddr");
#else
    return 0;
#endif
    if (!g_vk.GetInstanceProcAddr) {
        ps3rt_log("[rsx] Vulkan: vkGetInstanceProcAddr missing\n");
        return 0;
    }
    g_vk.CreateInstance = (PFN_vkCreateInstance)vk_ipa(nullptr, "vkCreateInstance");
    g_vk.EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vk_ipa(nullptr, "vkEnumeratePhysicalDevices");
    if (!g_vk.CreateInstance) {
        ps3rt_log("[rsx] Vulkan: vkCreateInstance missing\n");
        return 0;
    }
    return 1;
}

static void load_instance_fns() {
    void* i = g_vk.instance;
    g_vk.DestroyInstance = (PFN_vkDestroyInstance)vk_ipa(i, "vkDestroyInstance");
    g_vk.EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)vk_ipa(i, "vkEnumeratePhysicalDevices");
    g_vk.GetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vk_ipa(i, "vkGetPhysicalDeviceQueueFamilyProperties");
    g_vk.GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vk_ipa(i, "vkGetPhysicalDeviceSurfaceSupportKHR");
    g_vk.GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vk_ipa(i, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    g_vk.GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vk_ipa(i, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    g_vk.GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vk_ipa(i, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    g_vk.GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)vk_ipa(i, "vkGetPhysicalDeviceProperties");
    g_vk.GetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)vk_ipa(i, "vkGetPhysicalDeviceMemoryProperties");
    g_vk.CreateDevice = (PFN_vkCreateDevice)vk_ipa(i, "vkCreateDevice");
    g_vk.CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vk_ipa(i, "vkCreateWin32SurfaceKHR");
    g_vk.DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vk_ipa(i, "vkDestroySurfaceKHR");
}

static void load_device_fns() {
    void* d = g_vk.device;
    g_vk.DestroyDevice = (PFN_vkDestroyDevice)vk_dpa(d, "vkDestroyDevice");
    g_vk.GetDeviceQueue = (PFN_vkGetDeviceQueue)vk_dpa(d, "vkGetDeviceQueue");
    g_vk.CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vk_dpa(d, "vkCreateSwapchainKHR");
    g_vk.DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vk_dpa(d, "vkDestroySwapchainKHR");
    g_vk.GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vk_dpa(d, "vkGetSwapchainImagesKHR");
    g_vk.CreateImageView = (PFN_vkCreateImageView)vk_dpa(d, "vkCreateImageView");
    g_vk.DestroyImageView = (PFN_vkDestroyImageView)vk_dpa(d, "vkDestroyImageView");
    g_vk.CreateRenderPass = (PFN_vkCreateRenderPass)vk_dpa(d, "vkCreateRenderPass");
    g_vk.DestroyRenderPass = (PFN_vkDestroyRenderPass)vk_dpa(d, "vkDestroyRenderPass");
    g_vk.CreateFramebuffer = (PFN_vkCreateFramebuffer)vk_dpa(d, "vkCreateFramebuffer");
    g_vk.DestroyFramebuffer = (PFN_vkDestroyFramebuffer)vk_dpa(d, "vkDestroyFramebuffer");
    g_vk.CreateCommandPool = (PFN_vkCreateCommandPool)vk_dpa(d, "vkCreateCommandPool");
    g_vk.DestroyCommandPool = (PFN_vkDestroyCommandPool)vk_dpa(d, "vkDestroyCommandPool");
    g_vk.AllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)vk_dpa(d, "vkAllocateCommandBuffers");
    g_vk.BeginCommandBuffer = (PFN_vkBeginCommandBuffer)vk_dpa(d, "vkBeginCommandBuffer");
    g_vk.EndCommandBuffer = (PFN_vkEndCommandBuffer)vk_dpa(d, "vkEndCommandBuffer");
    g_vk.CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vk_dpa(d, "vkCmdBeginRenderPass");
    g_vk.CmdEndRenderPass = (PFN_vkCmdEndRenderPass)vk_dpa(d, "vkCmdEndRenderPass");
    g_vk.CmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vk_dpa(d, "vkCmdPipelineBarrier");
    g_vk.CmdClearColorImage = (PFN_vkCmdClearColorImage)vk_dpa(d, "vkCmdClearColorImage");
    g_vk.CreateSemaphore = (PFN_vkCreateSemaphore)vk_dpa(d, "vkCreateSemaphore");
    g_vk.DestroySemaphore = (PFN_vkDestroySemaphore)vk_dpa(d, "vkDestroySemaphore");
    g_vk.CreateFence = (PFN_vkCreateFence)vk_dpa(d, "vkCreateFence");
    g_vk.DestroyFence = (PFN_vkDestroyFence)vk_dpa(d, "vkDestroyFence");
    g_vk.WaitForFences = (PFN_vkWaitForFences)vk_dpa(d, "vkWaitForFences");
    g_vk.ResetFences = (PFN_vkResetFences)vk_dpa(d, "vkResetFences");
    g_vk.QueueSubmit = (PFN_vkQueueSubmit)vk_dpa(d, "vkQueueSubmit");
    g_vk.QueuePresentKHR = (PFN_vkQueuePresentKHR)vk_dpa(d, "vkQueuePresentKHR");
    g_vk.AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vk_dpa(d, "vkAcquireNextImageKHR");
    g_vk.DeviceWaitIdle = (PFN_vkDeviceWaitIdle)vk_dpa(d, "vkDeviceWaitIdle");
    g_vk.CreateBuffer = (PFN_vkCreateBuffer)vk_dpa(d, "vkCreateBuffer");
    g_vk.DestroyBuffer = (PFN_vkDestroyBuffer)vk_dpa(d, "vkDestroyBuffer");
    g_vk.GetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vk_dpa(d, "vkGetBufferMemoryRequirements");
    g_vk.AllocateMemory = (PFN_vkAllocateMemory)vk_dpa(d, "vkAllocateMemory");
    g_vk.FreeMemory = (PFN_vkFreeMemory)vk_dpa(d, "vkFreeMemory");
    g_vk.BindBufferMemory = (PFN_vkBindBufferMemory)vk_dpa(d, "vkBindBufferMemory");
    g_vk.MapMemory = (PFN_vkMapMemory)vk_dpa(d, "vkMapMemory");
    g_vk.UnmapMemory = (PFN_vkUnmapMemory)vk_dpa(d, "vkUnmapMemory");
    g_vk.CmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vk_dpa(d, "vkCmdCopyBufferToImage");
    g_vk.ResetCommandBuffer = (PFN_vkResetCommandBuffer)vk_dpa(d, "vkResetCommandBuffer");
}

static void destroy_swapchain_objects() {
    if (!g_vk.device) return;
    if (g_vk.DeviceWaitIdle) g_vk.DeviceWaitIdle(g_vk.device);
    for (auto fb : g_vk.framebuffers)
        if (fb && g_vk.DestroyFramebuffer) g_vk.DestroyFramebuffer(g_vk.device, fb, nullptr);
    g_vk.framebuffers.clear();
    for (auto v : g_vk.views)
        if (v && g_vk.DestroyImageView) g_vk.DestroyImageView(g_vk.device, v, nullptr);
    g_vk.views.clear();
    g_vk.images.clear();
    if (g_vk.swapchain && g_vk.DestroySwapchainKHR) {
        g_vk.DestroySwapchainKHR(g_vk.device, g_vk.swapchain, nullptr);
        g_vk.swapchain = nullptr;
    }
}

static int create_swapchain() {
    VkSurfaceCapabilitiesKHR caps{};
    if (g_vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(g_vk.phys, g_vk.surface, &caps) != VK_SUCCESS)
        return 0;

    uint32_t fc = 0;
    g_vk.GetPhysicalDeviceSurfaceFormatsKHR(g_vk.phys, g_vk.surface, &fc, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fc ? fc : 1);
    if (fc) g_vk.GetPhysicalDeviceSurfaceFormatsKHR(g_vk.phys, g_vk.surface, &fc, formats.data());
    g_vk.format = VK_FORMAT_B8G8R8A8_UNORM;
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) {
            g_vk.format = f.format;
            break;
        }
    }
    if (fc == 1 && formats[0].format == 0)
        g_vk.format = VK_FORMAT_B8G8R8A8_UNORM;

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == 0xFFFFFFFFu) {
        extent.width = g_vk.width;
        extent.height = g_vk.height;
    }
    if (extent.width < caps.minImageExtent.width) extent.width = caps.minImageExtent.width;
    if (extent.height < caps.minImageExtent.height) extent.height = caps.minImageExtent.height;
    if (caps.maxImageExtent.width && extent.width > caps.maxImageExtent.width) extent.width = caps.maxImageExtent.width;
    if (caps.maxImageExtent.height && extent.height > caps.maxImageExtent.height) extent.height = caps.maxImageExtent.height;
    g_vk.width = extent.width;
    g_vk.height = extent.height;

    uint32_t imgCount = caps.minImageCount + 1;
    if (caps.maxImageCount && imgCount > caps.maxImageCount) imgCount = caps.maxImageCount;

    int presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t pmc = 0;
    g_vk.GetPhysicalDeviceSurfacePresentModesKHR(g_vk.phys, g_vk.surface, &pmc, nullptr);
    std::vector<int> modes(pmc ? pmc : 1);
    if (pmc) g_vk.GetPhysicalDeviceSurfacePresentModesKHR(g_vk.phys, g_vk.surface, &pmc, modes.data());
    for (int m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) { presentMode = m; break; }

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = g_vk.surface;
    sci.minImageCount = imgCount;
    sci.imageFormat = g_vk.format;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = presentMode;
    sci.clipped = VK_TRUE;
    sci.oldSwapchain = nullptr;

    if (g_vk.CreateSwapchainKHR(g_vk.device, &sci, nullptr, &g_vk.swapchain) != VK_SUCCESS) {
        ps3rt_log("[rsx] Vulkan: CreateSwapchainKHR failed\n");
        return 0;
    }

    uint32_t nimg = 0;
    g_vk.GetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &nimg, nullptr);
    g_vk.images.resize(nimg);
    g_vk.GetSwapchainImagesKHR(g_vk.device, g_vk.swapchain, &nimg, g_vk.images.data());

    g_vk.views.resize(nimg);
    for (uint32_t i = 0; i < nimg; ++i) {
        VkImageViewCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image = g_vk.images[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = g_vk.format;
        vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.levelCount = 1;
        vi.subresourceRange.layerCount = 1;
        if (g_vk.CreateImageView(g_vk.device, &vi, nullptr, &g_vk.views[i]) != VK_SUCCESS)
            return 0;
    }

    if (!g_vk.render_pass) {
        VkAttachmentDescription att{};
        att.format = g_vk.format;
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
        if (g_vk.CreateRenderPass(g_vk.device, &rpci, nullptr, &g_vk.render_pass) != VK_SUCCESS)
            return 0;
    }

    g_vk.framebuffers.resize(nimg);
    for (uint32_t i = 0; i < nimg; ++i) {
        void* atts[] = { g_vk.views[i] };
        VkFramebufferCreateInfo fbci{};
        fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbci.renderPass = g_vk.render_pass;
        fbci.attachmentCount = 1;
        fbci.pAttachments = atts;
        fbci.width = g_vk.width;
        fbci.height = g_vk.height;
        fbci.layers = 1;
        if (g_vk.CreateFramebuffer(g_vk.device, &fbci, nullptr, &g_vk.framebuffers[i]) != VK_SUCCESS)
            return 0;
    }
    return 1;
}

int rsx_vk_init(void* hwnd, uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lk(g_vk_mu);
    if (g_vk.ready) return 1;
    if (!hwnd) return 0;
    g_vk.width = width ? width : 1280;
    g_vk.height = height ? height : 720;

    if (!load_vulkan_lib()) return 0;

    const char* exts[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "PPSX33";
    app.applicationVersion = 1;
    app.pEngineName = "PPSX33-RSX";
    app.engineVersion = 1;
    app.apiVersion = (1u << 22) | (2u << 12); // Vulkan 1.2

    VkInstanceCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = 2;
    ici.ppEnabledExtensionNames = exts;

    if (g_vk.CreateInstance(&ici, nullptr, &g_vk.instance) != VK_SUCCESS) {
        // Retry with 1.1 / 1.0 if 1.2 instance fails on old loaders
        app.apiVersion = (1u << 22) | (0u << 12);
        if (g_vk.CreateInstance(&ici, nullptr, &g_vk.instance) != VK_SUCCESS) {
            ps3rt_log("[rsx] Vulkan: CreateInstance failed\n");
            return 0;
        }
        ps3rt_log("[rsx] Vulkan: instance API 1.0 (loader fell back)\n");
    } else {
        ps3rt_log("[rsx] Vulkan: instance API 1.2\n");
    }
    load_instance_fns();

    VkWin32SurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hinstance = GetModuleHandleA(nullptr);
    sci.hwnd = hwnd;
    if (!g_vk.CreateWin32SurfaceKHR ||
        g_vk.CreateWin32SurfaceKHR(g_vk.instance, &sci, nullptr, &g_vk.surface) != VK_SUCCESS) {
        ps3rt_log("[rsx] Vulkan: Win32 surface failed\n");
        return 0;
    }

    uint32_t pd_count = 0;
    g_vk.EnumeratePhysicalDevices(g_vk.instance, &pd_count, nullptr);
    if (!pd_count) {
        ps3rt_log("[rsx] Vulkan: no physical devices\n");
        return 0;
    }
    std::vector<void*> phys(pd_count);
    g_vk.EnumeratePhysicalDevices(g_vk.instance, &pd_count, phys.data());

    int best = -1;
    int best_score = -1;
    for (uint32_t i = 0; i < pd_count; ++i) {
        uint32_t qcount = 0;
        g_vk.GetPhysicalDeviceQueueFamilyProperties(phys[i], &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(qcount);
        g_vk.GetPhysicalDeviceQueueFamilyProperties(phys[i], &qcount, qprops.data());
        for (uint32_t q = 0; q < qcount; ++q) {
            VkBool32 present = 0;
            g_vk.GetPhysicalDeviceSurfaceSupportKHR(phys[i], q, g_vk.surface, &present);
            if ((qprops[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                VkPhysicalDeviceProperties props{};
                g_vk.GetPhysicalDeviceProperties(phys[i], &props);
                int score = (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 100 : 10;
                if (score > best_score) {
                    best_score = score;
                    best = (int)i;
                    g_vk.qfamily = q;
                }
                break;
            }
        }
    }
    if (best < 0) {
        ps3rt_log("[rsx] Vulkan: no graphics+present queue\n");
        return 0;
    }
    g_vk.phys = phys[(size_t)best];
    {
        VkPhysicalDeviceProperties props{};
        g_vk.GetPhysicalDeviceProperties(g_vk.phys, &props);
        ps3rt_log("[rsx] Vulkan: device \"%s\" qfamily=%u\n", props.deviceName, g_vk.qfamily);
    }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = g_vk.qfamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    const char* dex[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = dex;
    if (g_vk.CreateDevice(g_vk.phys, &dci, nullptr, &g_vk.device) != VK_SUCCESS) {
        ps3rt_log("[rsx] Vulkan: CreateDevice failed\n");
        return 0;
    }
    load_device_fns();
    g_vk.GetDeviceQueue(g_vk.device, g_vk.qfamily, 0, &g_vk.queue);

    if (!create_swapchain()) return 0;

    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpci.queueFamilyIndex = g_vk.qfamily;
    if (g_vk.CreateCommandPool(g_vk.device, &cpci, nullptr, &g_vk.cmd_pool) != VK_SUCCESS)
        return 0;

    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = g_vk.cmd_pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;
    if (g_vk.AllocateCommandBuffers(g_vk.device, &cbai, &g_vk.cmd) != VK_SUCCESS)
        return 0;

    VkSemaphoreCreateInfo sci2{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
    VkFenceCreateInfo fci{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT };
    if (g_vk.CreateSemaphore(g_vk.device, &sci2, nullptr, &g_vk.image_available) != VK_SUCCESS) return 0;
    if (g_vk.CreateSemaphore(g_vk.device, &sci2, nullptr, &g_vk.render_finished) != VK_SUCCESS) return 0;
    if (g_vk.CreateFence(g_vk.device, &fci, nullptr, &g_vk.in_flight) != VK_SUCCESS) return 0;

    g_vk.ready = 1;
    ps3rt_log("[rsx] Vulkan 1.2 host present ready %ux%u images=%u\n",
        g_vk.width, g_vk.height, (unsigned)g_vk.images.size());
    return 1;
}

void rsx_vk_shutdown(void) {
    std::lock_guard<std::mutex> lk(g_vk_mu);
    if (!g_vk.device) {
        g_vk.ready = 0;
        return;
    }
    if (g_vk.DeviceWaitIdle) g_vk.DeviceWaitIdle(g_vk.device);
    destroy_swapchain_objects();
    if (g_vk.render_pass && g_vk.DestroyRenderPass) g_vk.DestroyRenderPass(g_vk.device, g_vk.render_pass, nullptr);
    g_vk.render_pass = nullptr;
    if (g_vk.image_available) g_vk.DestroySemaphore(g_vk.device, g_vk.image_available, nullptr);
    if (g_vk.render_finished) g_vk.DestroySemaphore(g_vk.device, g_vk.render_finished, nullptr);
    if (g_vk.in_flight) g_vk.DestroyFence(g_vk.device, g_vk.in_flight, nullptr);
    if (g_vk.cmd_pool) g_vk.DestroyCommandPool(g_vk.device, g_vk.cmd_pool, nullptr);
    g_vk.image_available = g_vk.render_finished = g_vk.in_flight = g_vk.cmd_pool = g_vk.cmd = nullptr;
    if (g_vk.DestroyDevice) g_vk.DestroyDevice(g_vk.device, nullptr);
    g_vk.device = g_vk.queue = nullptr;
    if (g_vk.surface && g_vk.DestroySurfaceKHR) g_vk.DestroySurfaceKHR(g_vk.instance, g_vk.surface, nullptr);
    g_vk.surface = nullptr;
    if (g_vk.instance && g_vk.DestroyInstance) g_vk.DestroyInstance(g_vk.instance, nullptr);
    g_vk.instance = nullptr;
    g_vk.ready = 0;
}

int rsx_vk_ready(void) { return g_vk.ready; }

int rsx_vk_present_clear(float r, float g, float b, float a) {
    if (!g_vk_mu.try_lock()) return 0; // skip frame if busy — never block HWND pump
    std::lock_guard<std::mutex> lk(g_vk_mu, std::adopt_lock);
    if (!g_vk.ready) return 0;

    g_vk.WaitForFences(g_vk.device, 1, &g_vk.in_flight, VK_TRUE, 50000000ull); /* 50ms */
    g_vk.ResetFences(g_vk.device, 1, &g_vk.in_flight);

    uint32_t imageIndex = 0;
    VkResult ar = g_vk.AcquireNextImageKHR(g_vk.device, g_vk.swapchain, 50000000ull,
        g_vk.image_available, nullptr, &imageIndex);
    if (ar == VK_ERROR_OUT_OF_DATE_KHR || ar == VK_SUBOPTIMAL_KHR) {
        destroy_swapchain_objects();
        if (!create_swapchain()) return 0;
        return 0;
    }
    if (ar != VK_SUCCESS) return 0;
    if (imageIndex >= g_vk.framebuffers.size()) return 0;

    if (g_vk.ResetCommandBuffer) g_vk.ResetCommandBuffer(g_vk.cmd, 0);
    VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr };
    g_vk.BeginCommandBuffer(g_vk.cmd, &begin);

    VkClearValue clear{};
    clear.color.float32[0] = r;
    clear.color.float32[1] = g;
    clear.color.float32[2] = b;
    clear.color.float32[3] = a;
    VkRenderPassBeginInfo rpbi{};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = g_vk.render_pass;
    rpbi.framebuffer = g_vk.framebuffers[imageIndex];
    rpbi.renderArea.extent.width = g_vk.width;
    rpbi.renderArea.extent.height = g_vk.height;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clear;
    g_vk.CmdBeginRenderPass(g_vk.cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    g_vk.CmdEndRenderPass(g_vk.cmd);
    g_vk.EndCommandBuffer(g_vk.cmd);

    VkFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &g_vk.image_available;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &g_vk.cmd;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &g_vk.render_finished;
    if (g_vk.QueueSubmit(g_vk.queue, 1, &si, g_vk.in_flight) != VK_SUCCESS) return 0;

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &g_vk.render_finished;
    pi.swapchainCount = 1;
    pi.pSwapchains = &g_vk.swapchain;
    pi.pImageIndices = &imageIndex;
    VkResult pr = g_vk.QueuePresentKHR(g_vk.queue, &pi);
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR) {
        destroy_swapchain_objects();
        create_swapchain();
    }
    return pr == VK_SUCCESS || pr == VK_SUBOPTIMAL_KHR;
}

static uint32_t vk_find_mem_type(uint32_t type_bits, uint32_t props) {
    VkPhysicalDeviceMemoryProperties mp{};
    g_vk.GetPhysicalDeviceMemoryProperties(g_vk.phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
        if ((type_bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    return 0;
}

int rsx_vk_present_guest_rgba(const void* pixels, uint32_t w, uint32_t h, uint32_t pitch_bytes) {
    std::lock_guard<std::mutex> lk(g_vk_mu);
    if (!g_vk.ready || !pixels || !w || !h) return 0;
    if (!pitch_bytes) pitch_bytes = w * 4u;

    g_vk.WaitForFences(g_vk.device, 1, &g_vk.in_flight, VK_TRUE, 100000000ull); /* 100ms */
    g_vk.ResetFences(g_vk.device, 1, &g_vk.in_flight);

    uint32_t imageIndex = 0;
    VkResult ar = g_vk.AcquireNextImageKHR(g_vk.device, g_vk.swapchain, UINT64_MAX,
        g_vk.image_available, nullptr, &imageIndex);
    if (ar == VK_ERROR_OUT_OF_DATE_KHR || ar == VK_SUBOPTIMAL_KHR) {
        destroy_swapchain_objects();
        if (!create_swapchain()) return 0;
        return 0;
    }
    if (ar != VK_SUCCESS || imageIndex >= g_vk.images.size()) return 0;

    VkDeviceSize buf_size = (VkDeviceSize)pitch_bytes * h;
    void* staging = nullptr;
    void* staging_mem = nullptr;
    {
        VkBufferCreateInfo bci{};
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size = buf_size;
        bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (g_vk.CreateBuffer(g_vk.device, &bci, nullptr, &staging) != VK_SUCCESS) return 0;
        VkMemoryRequirements mr{};
        g_vk.GetBufferMemoryRequirements(g_vk.device, staging, &mr);
        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = vk_find_mem_type(mr.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (g_vk.AllocateMemory(g_vk.device, &mai, nullptr, &staging_mem) != VK_SUCCESS) {
            g_vk.DestroyBuffer(g_vk.device, staging, nullptr);
            return 0;
        }
        g_vk.BindBufferMemory(g_vk.device, staging, staging_mem, 0);
        void* mapped = nullptr;
        g_vk.MapMemory(g_vk.device, staging_mem, 0, buf_size, 0, &mapped);
        if (mapped) {
            std::memcpy(mapped, pixels, (size_t)buf_size);
            g_vk.UnmapMemory(g_vk.device, staging_mem);
        }
    }

    if (g_vk.ResetCommandBuffer) g_vk.ResetCommandBuffer(g_vk.cmd, 0);
    VkCommandBufferBeginInfo begin{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr };
    g_vk.BeginCommandBuffer(g_vk.cmd, &begin);

    // UNDEFINED -> TRANSFER_DST
    VkImageMemoryBarrier to_dst{};
    to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_dst.srcAccessMask = 0;
    to_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_dst.srcQueueFamilyIndex = 0xFFFFFFFFu;
    to_dst.dstQueueFamilyIndex = 0xFFFFFFFFu;
    to_dst.image = g_vk.images[imageIndex];
    to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    to_dst.subresourceRange.levelCount = 1;
    to_dst.subresourceRange.layerCount = 1;
    g_vk.CmdPipelineBarrier(g_vk.cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &to_dst);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = pitch_bytes / 4u;
    region.bufferImageHeight = h;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = w < g_vk.width ? w : g_vk.width;
    region.imageExtent.height = h < g_vk.height ? h : g_vk.height;
    region.imageExtent.depth = 1;
    g_vk.CmdCopyBufferToImage(g_vk.cmd, staging, g_vk.images[imageIndex],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // TRANSFER_DST -> PRESENT
    VkImageMemoryBarrier to_present = to_dst;
    to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    g_vk.CmdPipelineBarrier(g_vk.cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &to_present);

    g_vk.EndCommandBuffer(g_vk.cmd);

    VkFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &g_vk.image_available;
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &g_vk.cmd;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &g_vk.render_finished;
    int ok = (g_vk.QueueSubmit(g_vk.queue, 1, &si, g_vk.in_flight) == VK_SUCCESS);

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &g_vk.render_finished;
    pi.swapchainCount = 1;
    pi.pSwapchains = &g_vk.swapchain;
    pi.pImageIndices = &imageIndex;
    g_vk.QueuePresentKHR(g_vk.queue, &pi);

    /* avoid DeviceWaitIdle — free staging after fence on next frame */
    g_vk.WaitForFences(g_vk.device, 1, &g_vk.in_flight, VK_TRUE, 100000000ull);
    g_vk.DestroyBuffer(g_vk.device, staging, nullptr);
    g_vk.FreeMemory(g_vk.device, staging_mem, nullptr);
    return ok;
}

void rsx_vk_poll_window(void) {
#if defined(_WIN32)
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
#endif
}
