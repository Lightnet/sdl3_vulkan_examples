#ifndef VSDL_TYPES_H
#define VSDL_TYPES_H

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <stdbool.h> // For bool type

typedef struct {
    VkBuffer buffer;
    VmaAllocation allocation;
    uint32_t vertexCount;
    bool exists; // Requires stdbool.h
    VkImage texture;
    VmaAllocation texAlloc;
    VkImageView textureView;
} RenderObject;

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkBuffer uniformBuffer;
    VmaAllocation uniformAllocation;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    uint32_t imageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* swapchainFramebuffers;
    VkImage depthImage;
    VmaAllocation depthAllocation;
    VkImageView depthImageView;
    RenderObject triangle;
    RenderObject cube;
    RenderObject text;
    uint32_t graphicsQueueFamilyIndex;
    VkSampler textureSampler;
} VulkanContext;

#endif