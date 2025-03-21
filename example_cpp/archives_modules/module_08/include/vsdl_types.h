// vsdl_types.h
#ifndef VSDL_TYPES_H
#define VSDL_TYPES_H

#include <SDL3/SDL.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <vector>

struct Vertex {
    float pos[2];  // x, y
    float color[3]; // r, g, b
};

struct UniformBufferObject {
    float transform[16]; // 4x4 matrix as flat array
};

struct VSDL_Context {
    SDL_Window* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsFamily;
    VmaAllocator allocator;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    VkBuffer vertexBuffer;
    VmaAllocation vertexBufferAllocation;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence frameFence;
    VkBuffer uniformBuffer;             // Added
    VmaAllocation uniformBufferAllocation; // Added
    VkDescriptorSetLayout descriptorSetLayout; // Added
    VkDescriptorSet descriptorSet;      // Added
    VkDescriptorPool descriptorPool;    // Added
};

#endif