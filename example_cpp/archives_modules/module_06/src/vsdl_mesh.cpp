// vsdl_mesh.cpp
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_log.h>
#include "vsdl_mesh.h"
#include "vsdl_types.h"

bool create_vertex_buffer(VSDL_Context& ctx) {
    Vertex vertices[] = {
        {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Top: Red
        {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // Bottom right: Green
        {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}  // Bottom left: Blue
    };
    VkDeviceSize bufferSize = sizeof(vertices);

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    if (vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &ctx.vertexBuffer, &ctx.vertexBufferAllocation, nullptr) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex buffer");
        return false;
    }

    void* data;
    vmaMapMemory(ctx.allocator, ctx.vertexBufferAllocation, &data);
    memcpy(data, vertices, bufferSize);
    vmaUnmapMemory(ctx.allocator, ctx.vertexBufferAllocation);
    SDL_Log("Vertex buffer created with VMA");

    return true;
}

bool create_uniform_buffer(VSDL_Context& ctx) {
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