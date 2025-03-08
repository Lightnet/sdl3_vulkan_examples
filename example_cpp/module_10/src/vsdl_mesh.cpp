// vsdl_mesh.cpp
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_log.h>
#include "vsdl_mesh.h"
#include "vsdl_types.h"

bool create_vertex_buffer(VSDL_Context& ctx) {
  static float offsetX = 0.0f; // Persistent offset
  Vertex vertices[] = {
      {{ 0.0f + offsetX, -0.25f}, {1.0f, 0.0f, 0.0f}}, // Top: Red
      {{ 0.25f + offsetX, 0.25f}, {0.0f, 1.0f, 0.0f}}, // Bottom right: Green
      {{-0.25f + offsetX, 0.25f}, {0.0f, 0.0f, 1.0f}}  // Bottom left: Blue
  };
  VkDeviceSize bufferSize = sizeof(vertices);

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  VkBuffer buffer;
  VmaAllocation allocation;
  if (vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex buffer");
      return false;
  }

  void* data;
  vmaMapMemory(ctx.allocator, allocation, &data);
  memcpy(data, vertices, bufferSize);
  vmaUnmapMemory(ctx.allocator, allocation);

  ctx.vertexBuffers.push_back({buffer, allocation});
  SDL_Log("Vertex buffer created with VMA (total: %zu)", ctx.vertexBuffers.size());
  offsetX += 0.5f; // Shift next triangle right by 0.5 units
  return true;
}

// bool create_vertex_buffer(VSDL_Context& ctx) {
//     SDL_Log("create_vertex_buffer");
//     Vertex vertices[] = {
//         {{ 0.0f, -0.25f}, {1.0f, 0.0f, 0.0f}}, // Top: Red
//         {{ 0.25f, 0.25f}, {0.0f, 1.0f, 0.0f}}, // Bottom right: Green
//         {{-0.25f, 0.25f}, {0.0f, 0.0f, 1.0f}}  // Bottom left: Blue
//     };
//     VkDeviceSize bufferSize = sizeof(vertices);

//     VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
//     bufferInfo.size = bufferSize;
//     bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//     bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

//     VmaAllocationCreateInfo allocInfo = {};
//     allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
//     allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

//     VkBuffer buffer;
//     VmaAllocation allocation;
//     if (vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
//         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex buffer");
//         return false;
//     }

//     void* data;
//     vmaMapMemory(ctx.allocator, allocation, &data);
//     memcpy(data, vertices, bufferSize);
//     vmaUnmapMemory(ctx.allocator, allocation);

//     ctx.vertexBuffers.push_back({buffer, allocation});
//     SDL_Log("Vertex buffer created with VMA (total: %zu)", ctx.vertexBuffers.size());
//     return true;
// }

bool destroy_vertex_buffer(VSDL_Context& ctx) {
    SDL_Log("destroy_vertex_buffer");
    if (ctx.vertexBuffers.empty()) {
        SDL_Log("No vertex buffers to destroy");
        return false;
    }

    auto [buffer, allocation] = ctx.vertexBuffers.back();
    vmaDestroyBuffer(ctx.allocator, buffer, allocation);
    ctx.vertexBuffers.pop_back();
    SDL_Log("Vertex buffer destroyed (remaining: %zu)", ctx.vertexBuffers.size());
    return true;
}

bool create_uniform_buffer(VSDL_Context& ctx) {
    SDL_Log("create_uniform_buffer");
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &ctx.uniformBuffer, &ctx.uniformBufferAllocation, nullptr) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create uniform buffer");
        return false;
    }
    SDL_Log("Uniform buffer created with VMA");
    return true;
}