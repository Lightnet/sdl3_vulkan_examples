#include "vsdl_mesh.h"
#include "vsdl_log.h"
#include "vsdl_vulkan_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

FT_Library ft_library = NULL;

/**
 * Finds a suitable memory type for Vulkan allocations
 * @param vkCtx Vulkan context containing the physical device
 * @param typeFilter Bitmask of allowed memory types
 * @param properties Desired memory properties
 * @return Memory type index
 */
 uint32_t vsdl_find_memory_type(VulkanContext* vkCtx, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(vkCtx->physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
          return i;
      }
  }
  vsdl_log("Failed to find suitable memory type!\n");
  exit(1);
}

/**
 * Creates a simple colored triangle
 */
 void vsdl_create_triangle(VulkanContext* vkCtx, RenderObject* triangle) {
  if (triangle->exists) {
      vsdl_log("Triangle already exists, skipping creation\n");
      return;
  }

  float vertices[] = {
      0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f,
     -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,
      0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f
  };

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = sizeof(vertices);
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &triangle->buffer, &triangle->allocation, NULL) != VK_SUCCESS) {
      vsdl_log("Failed to create triangle buffer with VMA\n");
      exit(1);
  }

  void* data;
  vmaMapMemory(allocator, triangle->allocation, &data);
  memcpy(data, vertices, sizeof(vertices));
  vmaUnmapMemory(allocator, triangle->allocation);

  triangle->vertexCount = 3;
  triangle->exists = true;
  vsdl_log("Triangle created with VMA\n");
}

/**
 * Destroys the triangle object
 */
 void vsdl_destroy_triangle(VulkanContext* vkCtx, RenderObject* triangle) {
  if (!triangle->exists) {
      vsdl_log("Triangle does not exist, skipping destruction\n");
      return;
  }

  vkDeviceWaitIdle(vkCtx->device);
  vmaDestroyBuffer(allocator, triangle->buffer, triangle->allocation);
  triangle->buffer = VK_NULL_HANDLE;
  triangle->allocation = VK_NULL_HANDLE;
  triangle->vertexCount = 0;
  triangle->exists = false;
  vsdl_log("Triangle destroyed with VMA\n");
}

/**
 * Creates a colored cube
 */
 void vsdl_create_cube(VulkanContext* vkCtx, RenderObject* cube){
    if (cube->exists) {
        vsdl_log("Cube already exists, skipping creation\n");
        return;
    }

    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f, -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,
        // Back face
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  -1.0f, -1.0f,  0.0f,
        // Left face
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f, -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  -1.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  -1.0f, -1.0f,  0.0f, -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,
        // Right face
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,
        // Top face
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f, -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f,  0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f,  0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  -1.0f, -1.0f,  0.0f
    };

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &cube->buffer, &cube->allocation, NULL) != VK_SUCCESS) {
        vsdl_log("Failed to create cube buffer with VMA\n");
        exit(1);
    }

    void* data;
    vmaMapMemory(allocator, cube->allocation, &data);
    memcpy(data, vertices, sizeof(vertices));
    vmaUnmapMemory(allocator, cube->allocation);

    cube->vertexCount = 36;
    cube->exists = true;
    vsdl_log("Cube created with VMA\n");
}

/**
 * Destroys the cube object
 */
 void vsdl_destroy_cube(VulkanContext* vkCtx, RenderObject* cube) {
  if (!cube->exists) {
      vsdl_log("Cube does not exist, skipping destruction\n");
      return;
  }

  vkDeviceWaitIdle(vkCtx->device);
  vmaDestroyBuffer(allocator, cube->buffer, cube->allocation);
  cube->buffer = VK_NULL_HANDLE;
  cube->allocation = VK_NULL_HANDLE;
  cube->vertexCount = 0;
  cube->exists = false;
  vsdl_log("Cube destroyed with VMA\n");
}

/**
 * Creates a textured plane displaying "Hello World" using FreeType
 */
 void vsdl_create_text(VulkanContext* vkCtx, RenderObject* text) {
  if (text->exists) {
      vsdl_log("Text already exists, skipping creation\n");
      return;
  }

  if (!ft_library) {
      if (FT_Init_FreeType(&ft_library) != 0) {
          vsdl_log("Failed to initialize FreeType\n");
          exit(1);
      }
  }

  FT_Face face;
  if (FT_New_Face(ft_library, "FiraSans-Bold.ttf", 0, &face) != 0) {
      vsdl_log("Failed to load font FiraSans-Bold.ttf - ensure it’s in the executable directory\n");
      exit(1);
  }
  FT_Set_Pixel_Sizes(face, 0, 48);

  const char* textString = "Hello World";
  int atlas_width = 0, atlas_height = 0;
  int max_bearing_y = 0;

  for (const char* c = textString; *c; c++) {
      if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
      atlas_width += face->glyph->bitmap.width + 2;
      atlas_height = (face->glyph->bitmap.rows > atlas_height) ? face->glyph->bitmap.rows : atlas_height;
      max_bearing_y = (face->glyph->bitmap_top > max_bearing_y) ? face->glyph->bitmap_top : max_bearing_y;
  }

  int baseline_y = max_bearing_y;
  atlas_height += max_bearing_y;

  unsigned char* atlas_data = calloc(1, atlas_width * atlas_height);
  int x_offset = 0;

  for (const char* c = textString; *c; c++) {
      if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
      FT_Bitmap* bitmap = &face->glyph->bitmap;
      int y_offset = baseline_y - face->glyph->bitmap_top;
      for (unsigned int y = 0; y < bitmap->rows; y++) {
          for (unsigned int x = 0; x < bitmap->width; x++) {
              int atlas_y = y_offset + y;
              if (atlas_y >= 0 && atlas_y < atlas_height) {
                  atlas_data[(atlas_y * atlas_width) + x_offset + x] = bitmap->buffer[y * bitmap->width + x];
              }
          }
      }
      x_offset += bitmap->width + 2;
  }

  vsdl_log("Atlas dimensions: %d x %d, Baseline at y=%d\n", atlas_width, atlas_height, baseline_y);

  VkBuffer stagingBuffer;
  VmaAllocation stagingAlloc;
  VkBufferCreateInfo stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufferInfo.size = atlas_width * atlas_height;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo stagingAllocInfo = {};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, NULL) != VK_SUCCESS) {
      vsdl_log("Failed to create staging buffer for text texture\n");
      exit(1);
  }

  void* data;
  vmaMapMemory(allocator, stagingAlloc, &data);
  memcpy(data, atlas_data, atlas_width * atlas_height);
  vmaUnmapMemory(allocator, stagingAlloc);

  VkImageCreateInfo textImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  textImageInfo.imageType = VK_IMAGE_TYPE_2D;
  textImageInfo.format = VK_FORMAT_R8_UNORM;
  textImageInfo.extent.width = atlas_width;
  textImageInfo.extent.height = atlas_height;
  textImageInfo.extent.depth = 1;
  textImageInfo.mipLevels = 1;
  textImageInfo.arrayLayers = 1;
  textImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  textImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  textImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  textImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VmaAllocationCreateInfo texAllocInfo = {};
  texAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  if (vmaCreateImage(allocator, &textImageInfo, &texAllocInfo, &text->texture, &text->texAlloc, NULL) != VK_SUCCESS) {
      vsdl_log("Failed to create text texture with VMA\n");
      exit(1);
  }

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAllocInfo.commandPool = vkCtx->commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(vkCtx->device, &cmdAllocInfo, &commandBuffer) != VK_SUCCESS) {
      vsdl_log("Failed to allocate command buffer for text texture\n");
      exit(1);
  }

  VkCommandBufferBeginInfo cmdBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);

  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = text->texture;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  VkBufferImageCopy copyRegion = {};
  copyRegion.bufferOffset = 0;
  copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount = 1;
  copyRegion.imageExtent.width = atlas_width;
  copyRegion.imageExtent.height = atlas_height;
  copyRegion.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, text->texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  vkQueueSubmit(vkCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(vkCtx->graphicsQueue);

  vkFreeCommandBuffers(vkCtx->device, vkCtx->commandPool, 1, &commandBuffer);
  vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);

  VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = text->texture;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(vkCtx->device, &viewInfo, NULL, &text->textureView) != VK_SUCCESS) {
      vsdl_log("Failed to create text texture view\n");
      exit(1);
  }

  // Update descriptor set with text texture (binding 1)
  VkDescriptorImageInfo textDescriptorImageInfo = {};
  textDescriptorImageInfo.sampler = vkCtx->textureSampler; // Now valid
  textDescriptorImageInfo.imageView = text->textureView;
  textDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = vkCtx->descriptorSet;
  descriptorWrite.dstBinding = 1;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &textDescriptorImageInfo;

  vkUpdateDescriptorSets(vkCtx->device, 1, &descriptorWrite, 0, NULL);

  float baseline_v = (float)baseline_y / atlas_height;
  float vertices[] = {
      -0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, baseline_v,  1.0f,
      -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,       1.0f,
       0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, baseline_v,  1.0f,
      -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,       1.0f,
       0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,       1.0f,
       0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, baseline_v,  1.0f
  };

  VkBufferCreateInfo textBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  textBufferInfo.size = sizeof(vertices);
  textBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  textBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo textAllocInfo = {};
  textAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  if (vmaCreateBuffer(allocator, &textBufferInfo, &textAllocInfo, &text->buffer, &text->allocation, NULL) != VK_SUCCESS) {
      vsdl_log("Failed to create text buffer with VMA\n");
      exit(1);
  }

  vmaMapMemory(allocator, text->allocation, &data);
  memcpy(data, vertices, sizeof(vertices));
  vmaUnmapMemory(allocator, text->allocation);

  text->vertexCount = 6;
  text->exists = true;
  vsdl_log("Text 'Hello World' created with VMA\n");

  free(atlas_data);
  FT_Done_Face(face);
}

/**
 * Destroys the text object and resets descriptor to dummy texture
 */
 void vsdl_destroy_text(VulkanContext* vkCtx, RenderObject* text) {
  if (!text->exists) {
      vsdl_log("Text does not exist, skipping destruction\n");
      return;
  }

  vkDeviceWaitIdle(vkCtx->device);
  vkDestroyImageView(vkCtx->device, text->textureView, NULL);
  vmaDestroyImage(allocator, text->texture, text->texAlloc);
  vmaDestroyBuffer(allocator, text->buffer, text->allocation);

  VkDescriptorImageInfo dummyDescriptorImageInfo = {};
  dummyDescriptorImageInfo.sampler = vkCtx->textureSampler;
  dummyDescriptorImageInfo.imageView = dummyTextureView; // Now valid
  dummyDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = vkCtx->descriptorSet;
  descriptorWrite.dstBinding = 1;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &dummyDescriptorImageInfo;

  vkUpdateDescriptorSets(vkCtx->device, 1, &descriptorWrite, 0, NULL);

  text->buffer = VK_NULL_HANDLE;
  text->allocation = VK_NULL_HANDLE;
  text->texture = VK_NULL_HANDLE;
  text->texAlloc = VK_NULL_HANDLE;
  text->textureView = VK_NULL_HANDLE;
  text->vertexCount = 0;
  text->exists = false;
  vsdl_log("Text destroyed with VMA\n");
}