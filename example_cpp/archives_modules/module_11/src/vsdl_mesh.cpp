// vsdl_mesh.cpp
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_log.h>
#include "vsdl_mesh.h"
#include "vsdl_types.h"

bool create_triangle_buffer(VSDL_Context& ctx) {
  static float offsetX = 0.0f;
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

  Mesh mesh;
  mesh.type = MeshType::TRIANGLE;
  if (vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &mesh.vertexBuffer, &mesh.vertexAllocation, nullptr) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create triangle vertex buffer");
      return false;
  }

  void* data;
  vmaMapMemory(ctx.allocator, mesh.vertexAllocation, &data);
  memcpy(data, vertices, bufferSize);
  vmaUnmapMemory(ctx.allocator, mesh.vertexAllocation);

  ctx.meshes.push_back(mesh);
  SDL_Log("Triangle buffer created with VMA (total: %zu)", ctx.meshes.size());
  offsetX += 0.5f;
  return true;
}

bool create_plane_buffer(VSDL_Context& ctx) {
  static float offsetX = 0.0f;
  Vertex vertices[] = {
      {{-0.25f + offsetX, -0.25f}, {1.0f, 0.0f, 0.0f}}, // Top-left: Red
      {{ 0.25f + offsetX, -0.25f}, {0.0f, 1.0f, 0.0f}}, // Top-right: Green
      {{ 0.25f + offsetX,  0.25f}, {0.0f, 0.0f, 1.0f}}, // Bottom-right: Blue
      {{-0.25f + offsetX,  0.25f}, {1.0f, 1.0f, 0.0f}}  // Bottom-left: Yellow
  };
  uint32_t indices[] = {0, 1, 2, 2, 3, 0}; // Two triangles
  VkDeviceSize vertexSize = sizeof(vertices);
  VkDeviceSize indexSize = sizeof(indices);

  VkBufferCreateInfo vertexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  vertexBufferInfo.size = vertexSize;
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBufferCreateInfo indexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  indexBufferInfo.size = indexSize;
  indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

  Mesh mesh;
  mesh.type = MeshType::PLANE;
  mesh.indexCount = 6;

  if (vmaCreateBuffer(ctx.allocator, &vertexBufferInfo, &allocInfo, &mesh.vertexBuffer, &mesh.vertexAllocation, nullptr) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create plane vertex buffer");
      return false;
  }
  if (vmaCreateBuffer(ctx.allocator, &indexBufferInfo, &allocInfo, &mesh.indexBuffer, &mesh.indexAllocation, nullptr) != VK_SUCCESS) {
      vmaDestroyBuffer(ctx.allocator, mesh.vertexBuffer, mesh.vertexAllocation);
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create plane index buffer");
      return false;
  }

  void* data;
  vmaMapMemory(ctx.allocator, mesh.vertexAllocation, &data);
  memcpy(data, vertices, vertexSize);
  vmaUnmapMemory(ctx.allocator, mesh.vertexAllocation);

  vmaMapMemory(ctx.allocator, mesh.indexAllocation, &data);
  memcpy(data, indices, indexSize);
  vmaUnmapMemory(ctx.allocator, mesh.indexAllocation);

  ctx.meshes.push_back(mesh);
  SDL_Log("Plane buffer created with VMA (total: %zu)", ctx.meshes.size());
  offsetX += 0.5f;
  return true;
}

bool destroy_mesh(VSDL_Context& ctx) {
  if (ctx.meshes.empty()) {
      SDL_Log("No meshes to destroy");
      return false;
  }

  Mesh& mesh = ctx.meshes.back();
  vmaDestroyBuffer(ctx.allocator, mesh.vertexBuffer, mesh.vertexAllocation);
  if (mesh.indexBuffer) {
      vmaDestroyBuffer(ctx.allocator, mesh.indexBuffer, mesh.indexAllocation);
  }
  ctx.meshes.pop_back();
  SDL_Log("Mesh destroyed (remaining: %zu)", ctx.meshes.size());
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