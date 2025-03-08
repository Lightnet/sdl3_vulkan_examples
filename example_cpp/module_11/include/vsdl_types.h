// vsdl_types.h
#ifndef VSDL_TYPES_H
#define VSDL_TYPES_H
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h> // Add this for VmaAllocator and VmaAllocation
#include <vector>

struct Vertex {
  float pos[2];
  float color[3];
};

struct UniformBufferObject {
  float transform[16];
};

enum class MeshType { TRIANGLE, PLANE };

struct Mesh {
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VmaAllocation vertexAllocation = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE; // Optional, used for planes
  VmaAllocation indexAllocation = VK_NULL_HANDLE;
  uint32_t indexCount = 0; // 0 for triangles, 6 for planes
  MeshType type = MeshType::TRIANGLE;
};

struct VSDL_Context {
  SDL_Window* window = nullptr;
  VkInstance instance = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VmaAllocator allocator = VK_NULL_HANDLE;
  uint32_t graphicsFamily = 0;
  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  std::vector<VkImage> swapchainImages;
  VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D swapchainExtent = {0, 0};
  std::vector<VkImageView> swapchainImageViews;
  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> framebuffers;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<Mesh> meshes; // Replaces vertexBuffers
  VkBuffer uniformBuffer = VK_NULL_HANDLE;
  VmaAllocation uniformBufferAllocation = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
  VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
  VkFence frameFence = VK_NULL_HANDLE;
};

#endif