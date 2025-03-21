#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <vk_mem_alloc.h>
#include <spirv_cross_c.h>
#include "vulkan_init.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#define WIDTH 800
#define HEIGHT 600

typedef struct {
    vec3 pos;
    vec3 front;
    vec3 up;
    float yaw;
    float pitch;
} Camera;

typedef struct {
    VkBuffer buffer;
    VmaAllocation allocation;
    uint32_t vertexCount;
    bool exists;
    VkImage texture;          // Added for text texture
    VmaAllocation texAlloc;   // Texture allocation
    VkImageView textureView;  // Texture view
} RenderObject;

struct VulkanContext {
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
} vkCtx = {0};

typedef struct {
    mat4 model;
    mat4 view;
    mat4 proj;
} UBO;

FT_Library ft_library;

uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkCtx.physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    fprintf(stderr, "Failed to find suitable memory type!\n");
    exit(1);
}

void update_uniform_buffer(Camera* cam, float rotationAngle) {
    UBO ubo;
    glm_mat4_identity(ubo.model);
    glm_rotate_y(ubo.model, glm_rad(rotationAngle), ubo.model);
    glm_lookat(cam->pos, (vec3){cam->pos[0] + cam->front[0], cam->pos[1] + cam->front[1], cam->pos[2] + cam->front[2]}, cam->up, ubo.view);
    glm_perspective(glm_rad(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f, ubo.proj);
    ubo.proj[1][1] *= -1;

    void* data;
    vmaMapMemory(allocator, vkCtx.uniformAllocation, &data);
    memcpy(data, &ubo, sizeof(UBO));
    vmaUnmapMemory(allocator, vkCtx.uniformAllocation);
}

void create_triangle() {
    if (vkCtx.triangle.exists) {
        printf("Triangle already exists, skipping creation\n");
        return;
    }

    // Position (vec3), Color (vec3), TexCoord (vec2), TexFlag (float)
    float vertices[] = {
        0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,  0.0f, // Top (red)
       -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  -1.0f, -1.0f,  0.0f, // Bottom-left (green)
        0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  -1.0f, -1.0f,  0.0f  // Bottom-right (blue)
    };

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vkCtx.triangle.buffer, &vkCtx.triangle.allocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create triangle buffer with VMA\n");
        exit(1);
    }

    void* data;
    vmaMapMemory(allocator, vkCtx.triangle.allocation, &data);
    memcpy(data, vertices, sizeof(vertices));
    vmaUnmapMemory(allocator, vkCtx.triangle.allocation);

    vkCtx.triangle.vertexCount = 3;
    vkCtx.triangle.exists = true;
    printf("Triangle created with VMA\n");
}

void destroy_triangle() {
    if (!vkCtx.triangle.exists) {
        printf("Triangle does not exist, skipping destruction\n");
        return;
    }

    vkDeviceWaitIdle(vkCtx.device);
    vmaDestroyBuffer(allocator, vkCtx.triangle.buffer, vkCtx.triangle.allocation);
    vkCtx.triangle.buffer = VK_NULL_HANDLE;
    vkCtx.triangle.allocation = VK_NULL_HANDLE;
    vkCtx.triangle.vertexCount = 0;
    vkCtx.triangle.exists = false;
    printf("Triangle destroyed with VMA\n");
}

void create_cube() {
    if (vkCtx.cube.exists) {
        printf("Cube already exists, skipping creation\n");
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

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &vkCtx.cube.buffer, &vkCtx.cube.allocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create cube buffer with VMA\n");
        exit(1);
    }

    void* data;
    vmaMapMemory(allocator, vkCtx.cube.allocation, &data);
    memcpy(data, vertices, sizeof(vertices));
    vmaUnmapMemory(allocator, vkCtx.cube.allocation);

    vkCtx.cube.vertexCount = 36;
    vkCtx.cube.exists = true;
    printf("Cube created with VMA\n");
}

void destroy_cube() {
    if (!vkCtx.cube.exists) {
        printf("Cube does not exist, skipping destruction\n");
        return;
    }

    vkDeviceWaitIdle(vkCtx.device);
    vmaDestroyBuffer(allocator, vkCtx.cube.buffer, vkCtx.cube.allocation);
    vkCtx.cube.buffer = VK_NULL_HANDLE;
    vkCtx.cube.allocation = VK_NULL_HANDLE;
    vkCtx.cube.vertexCount = 0;
    vkCtx.cube.exists = false;
    printf("Cube destroyed with VMA\n");
}

void create_text() {
    if (vkCtx.text.exists) {
        printf("Text already exists, skipping creation\n");
        return;
    }

    if (!ft_library) {
        if (FT_Init_FreeType(&ft_library) != 0) {
            fprintf(stderr, "Failed to initialize FreeType\n");
            exit(1);
        }
    }

    FT_Face face;
    if (FT_New_Face(ft_library, "FiraSans-Bold.ttf", 0, &face) != 0) {
        fprintf(stderr, "Failed to load font FiraSans-Bold.ttf\n");
        exit(1);
    }
    FT_Set_Pixel_Sizes(face, 0, 48);

    const char* text = "Hello World";
    int atlas_width = 0, atlas_height = 0;
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
        atlas_width += face->glyph->bitmap.width + 2;
        atlas_height = (face->glyph->bitmap.rows > atlas_height) ? face->glyph->bitmap.rows : atlas_height;
    }

    unsigned char* atlas_data = calloc(1, atlas_width * atlas_height);
    int x_offset = 0;
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
        FT_Bitmap* bitmap = &face->glyph->bitmap;
        for (unsigned int y = 0; y < bitmap->rows; y++) {
            for (unsigned int x = 0; x < bitmap->width; x++) {
                atlas_data[(y * atlas_width) + x_offset + x] = bitmap->buffer[y * bitmap->width + x];
            }
        }
        x_offset += bitmap->width + 2;
    }

    VkBuffer stagingBuffer;
    VmaAllocation stagingAlloc;
    VkBufferCreateInfo stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    stagingBufferInfo.size = atlas_width * atlas_height;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create staging buffer for text texture\n");
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
    if (vmaCreateImage(allocator, &textImageInfo, &texAllocInfo, &vkCtx.text.texture, &vkCtx.text.texAlloc, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create text texture with VMA\n");
        exit(1);
    }

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = vkCtx.commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkCtx.device, &cmdAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo cmdBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkCtx.text.texture;
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

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vkCtx.text.texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkCtx.graphicsQueue);

    vkFreeCommandBuffers(vkCtx.device, vkCtx.commandPool, 1, &commandBuffer);
    vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);

    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = vkCtx.text.texture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vkCtx.device, &viewInfo, NULL, &vkCtx.text.textureView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create text texture view\n");
        exit(1);
    }

    // Update descriptor set with texture (binding 1)
    VkDescriptorImageInfo textDescriptorImageInfo = {};
    textDescriptorImageInfo.sampler = vkCtx.textureSampler;
    textDescriptorImageInfo.imageView = vkCtx.text.textureView;
    textDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = vkCtx.descriptorSet;
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &textDescriptorImageInfo;

    vkUpdateDescriptorSets(vkCtx.device, 1, &descriptorWrite, 0, NULL);

    // Vertex buffer for text: Position (vec3), Color (vec3), TexCoord (vec2), TexFlag (float)
    float vertices[] = {
        -0.5f, -0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  1.0f, // Bottom-left (white, textured)
         0.5f, -0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  1.0f, // Bottom-right (white, textured)
        -0.5f,  0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (white, textured)
        -0.5f,  0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (white, textured)
         0.5f, -0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  1.0f, // Bottom-right (white, textured)
         0.5f,  0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  1.0f  // Top-right (white, textured)
    };

    VkBufferCreateInfo textBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    textBufferInfo.size = sizeof(vertices);
    textBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    textBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo textAllocInfo = {};
    textAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &textBufferInfo, &textAllocInfo, &vkCtx.text.buffer, &vkCtx.text.allocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create text buffer with VMA\n");
        exit(1);
    }

    vmaMapMemory(allocator, vkCtx.text.allocation, &data);
    memcpy(data, vertices, sizeof(vertices));
    vmaUnmapMemory(allocator, vkCtx.text.allocation);

    vkCtx.text.vertexCount = 6;
    vkCtx.text.exists = true;
    printf("Text 'Hello World' created with VMA\n");

    free(atlas_data);
    FT_Done_Face(face);
}

void destroy_text() {
    if (!vkCtx.text.exists) {
        printf("Text does not exist, skipping destruction\n");
        return;
    }

    vkDeviceWaitIdle(vkCtx.device);
    vkDestroyImageView(vkCtx.device, vkCtx.text.textureView, NULL);
    vmaDestroyImage(allocator, vkCtx.text.texture, vkCtx.text.texAlloc);
    vmaDestroyBuffer(allocator, vkCtx.text.buffer, vkCtx.text.allocation);
    vkCtx.text.buffer = VK_NULL_HANDLE;
    vkCtx.text.allocation = VK_NULL_HANDLE;
    vkCtx.text.texture = VK_NULL_HANDLE;
    vkCtx.text.texAlloc = VK_NULL_HANDLE;
    vkCtx.text.textureView = VK_NULL_HANDLE;
    vkCtx.text.vertexCount = 0;
    vkCtx.text.exists = false;
    printf("Text destroyed with VMA\n");
}

void reflect_vertex_inputs(const char* shaderCode, size_t codeSize, VkVertexInputAttributeDescription** attrDesc, uint32_t* attrCount, uint32_t* stride) {
    spvc_context context = NULL;
    spvc_parsed_ir parsed_ir = NULL;
    spvc_compiler compiler = NULL;
    spvc_resources resources = NULL;
    const spvc_reflected_resource* stage_inputs = NULL;
    size_t input_count = 0;

    if (spvc_context_create(&context) != SPVC_SUCCESS) {
        fprintf(stderr, "Failed to create SPIRV-Cross context\n");
        exit(1);
    }

    if (spvc_context_parse_spirv(context, (const uint32_t*)shaderCode, codeSize / sizeof(uint32_t), &parsed_ir) != SPVC_SUCCESS) {
        fprintf(stderr, "Failed to parse SPIR-V\n");
        spvc_context_destroy(context);
        exit(1);
    }

    if (spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, parsed_ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler) != SPVC_SUCCESS) {
        fprintf(stderr, "Failed to create SPIRV-Cross compiler\n");
        spvc_context_destroy(context);
        exit(1);
    }

    if (spvc_compiler_create_shader_resources(compiler, &resources) != SPVC_SUCCESS) {
        fprintf(stderr, "Failed to create shader resources\n");
        spvc_context_destroy(context);
        exit(1);
    }

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &stage_inputs, &input_count) != SPVC_SUCCESS) {
        fprintf(stderr, "Failed to get stage inputs\n");
        spvc_context_destroy(context);
        exit(1);
    }

    *attrCount = (uint32_t)input_count;
    *attrDesc = malloc(*attrCount * sizeof(VkVertexInputAttributeDescription));
    *stride = 0;

    for (uint32_t i = 0; i < *attrCount; i++) {
        uint32_t location = spvc_compiler_get_decoration(compiler, stage_inputs[i].id, SpvDecorationLocation);
        spvc_type_id type_id = stage_inputs[i].type_id;
#pragma warning(push)
#pragma warning(disable: 4047)
        spvc_type type = spvc_compiler_get_type_handle(compiler, type_id);
#pragma warning(pop)

        (*attrDesc)[i].location = location;
        (*attrDesc)[i].binding = 0;
        (*attrDesc)[i].offset = *stride;

        spvc_basetype base_type = spvc_type_get_basetype(type);
        if (base_type == SPVC_BASETYPE_FP32) {
            unsigned vector_size = spvc_type_get_vector_size(type);
            switch (vector_size) {
                case 1: (*attrDesc)[i].format = VK_FORMAT_R32_SFLOAT; break;    // TexFlag
                case 2: (*attrDesc)[i].format = VK_FORMAT_R32G32_SFLOAT; break; // TexCoord
                case 3: (*attrDesc)[i].format = VK_FORMAT_R32G32B32_SFLOAT; break; // Position or Color
                default: fprintf(stderr, "Unsupported vector size: %u\n", vector_size); exit(1);
            }
            *stride += vector_size * sizeof(float);
        } else {
            fprintf(stderr, "Unsupported base type: %u\n", base_type);
            exit(1);
        }

        printf("Vertex input %u: location=%u, format=%u, offset=%u\n", 
               i, (*attrDesc)[i].location, (*attrDesc)[i].format, (*attrDesc)[i].offset);
    }

    spvc_context_destroy(context);
}

void create_pipeline() {
    printf("Attempting to load shaders...\n");
    FILE* vertFile = fopen("vert.spv", "rb");
    if (!vertFile) {
        fprintf(stderr, "Failed to open vert.spv - ensure it’s in the Debug directory\n");
        exit(1);
    }
    fseek(vertFile, 0, SEEK_END);
    long vertSize = ftell(vertFile);
    fseek(vertFile, 0, SEEK_SET);
    char* vertShaderCode = malloc(vertSize);
    fread(vertShaderCode, 1, vertSize, vertFile);
    fclose(vertFile);
    printf("Vertex shader loaded successfully\n");

    FILE* fragFile = fopen("frag.spv", "rb");
    if (!fragFile) {
        fprintf(stderr, "Failed to open frag.spv - ensure it’s in the Debug directory\n");
        exit(1);
    }
    fseek(fragFile, 0, SEEK_END);
    long fragSize = ftell(fragFile);
    fseek(fragFile, 0, SEEK_SET);
    char* fragShaderCode = malloc(fragSize);
    fread(fragShaderCode, 1, fragSize, fragFile);
    fclose(fragFile);
    printf("Fragment shader loaded successfully\n");

    VkVertexInputAttributeDescription* attrDesc = NULL;
    uint32_t attrCount = 0;
    uint32_t stride = 0;
    reflect_vertex_inputs(vertShaderCode, vertSize, &attrDesc, &attrCount, &stride);

    VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertShaderInfo.codeSize = vertSize;
    vertShaderInfo.pCode = (uint32_t*)vertShaderCode;
    VkShaderModule vertModule;
    if (vkCreateShaderModule(vkCtx.device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader module\n");
        exit(1);
    }

    VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragShaderInfo.codeSize = fragSize;
    fragShaderInfo.pCode = (uint32_t*)fragShaderCode;
    VkShaderModule fragModule;
    if (vkCreateShaderModule(vkCtx.device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
    };

    VkVertexInputBindingDescription bindingDesc = {0, stride, VK_VERTEX_INPUT_RATE_VERTEX};
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {WIDTH, HEIGHT}};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDescriptorSetLayoutBinding layoutBindings[2] = {};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = layoutBindings;

    if (vkCreateDescriptorSetLayout(vkCtx.device, &layoutInfo, NULL, &vkCtx.descriptorSetLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        exit(1);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vkCtx.descriptorSetLayout;

    if (vkCreatePipelineLayout(vkCtx.device, &pipelineLayoutInfo, NULL, &vkCtx.pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = vkCtx.pipelineLayout;
    pipelineInfo.renderPass = vkCtx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkCtx.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx.graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vkCtx.device, fragModule, NULL);
    vkDestroyShaderModule(vkCtx.device, vertModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
    free(attrDesc);
    printf("Graphics pipeline created successfully\n");
}

void record_command_buffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(vkCtx.commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin command buffer\n");
        exit(1);
    }

    VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = vkCtx.renderPass;
    renderPassInfo.framebuffer = vkCtx.swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = (VkExtent2D){WIDTH, HEIGHT};
    VkClearValue clearValues[2] = {{{{0.5f, 0.5f, 0.5f, 1.0f}}}, {{{1.0f, 0}}}};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(vkCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.graphicsPipeline);
    vkCmdBindDescriptorSets(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.pipelineLayout, 0, 1, &vkCtx.descriptorSet, 0, NULL);

    VkDeviceSize offsets[] = {0};
    if (vkCtx.triangle.exists) {
        printf("Rendering triangle with %u vertices\n", vkCtx.triangle.vertexCount);
        vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.triangle.buffer, offsets);
        vkCmdDraw(vkCtx.commandBuffer, vkCtx.triangle.vertexCount, 1, 0, 0);
    }
    if (vkCtx.cube.exists) {
        printf("Rendering cube with %u vertices\n", vkCtx.cube.vertexCount);
        vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.cube.buffer, offsets);
        vkCmdDraw(vkCtx.commandBuffer, vkCtx.cube.vertexCount, 1, 0, 0);
    }
    if (vkCtx.text.exists) {
        printf("Rendering text with %u vertices\n", vkCtx.text.vertexCount);
        vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.text.buffer, offsets);
        vkCmdDraw(vkCtx.commandBuffer, vkCtx.text.vertexCount, 1, 0, 0);
    }

    vkCmdEndRenderPass(vkCtx.commandBuffer);
    if (vkEndCommandBuffer(vkCtx.commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to end command buffer\n");
        exit(1);
    }
}

void reset_camera(Camera* cam) {
    cam->pos[0] = 0.0f; cam->pos[1] = 0.0f; cam->pos[2] = 3.0f;
    cam->front[0] = 0.0f; cam->front[1] = 0.0f; cam->front[2] = -1.0f;
    cam->up[0] = 0.0f; cam->up[1] = 1.0f; cam->up[2] = 0.0f;
    cam->yaw = -90.0f;
    cam->pitch = 0.0f;
    printf("Camera reset: Pos [0, 0, 3], Yaw -90, Pitch 0\n");
}

void update_camera(Camera* cam, SDL_Event* event, bool* mouseCaptured, SDL_Window* window, Uint32 deltaTime) {
    const float speed = 2.5f;
    float moveSpeed = speed * (deltaTime / 1000.0f);

    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.key) {
            case SDLK_W: {
                vec3 move; glm_vec3_scale(cam->front, moveSpeed, move); glm_vec3_add(cam->pos, move, cam->pos);
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_S: {
                vec3 move; glm_vec3_scale(cam->front, -moveSpeed, move); glm_vec3_add(cam->pos, move, cam->pos);
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_A: {
                vec3 left; glm_vec3_cross(cam->front, cam->up, left); glm_vec3_normalize(left); glm_vec3_scale(left, -moveSpeed, left); glm_vec3_add(cam->pos, left, cam->pos);
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_D: {
                vec3 right; glm_vec3_cross(cam->front, cam->up, right); glm_vec3_normalize(right); glm_vec3_scale(right, moveSpeed, right); glm_vec3_add(cam->pos, right, cam->pos);
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_ESCAPE: {
                *mouseCaptured = false;
                if (!SDL_SetWindowRelativeMouseMode(window, false)) {
                    printf("Failed to disable relative mouse mode: %s\n", SDL_GetError());
                }
                break;
            }
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        *mouseCaptured = true;
        if (!SDL_SetWindowRelativeMouseMode(window, true)) {
            printf("Failed to enable relative mouse mode: %s\n", SDL_GetError());
        }
    }
    if (*mouseCaptured && event->type == SDL_EVENT_MOUSE_MOTION) {
        float sensitivity = 0.1f;
        cam->yaw += event->motion.xrel * sensitivity;
        cam->pitch -= event->motion.yrel * sensitivity;
        if (cam->pitch > 89.0f) cam->pitch = 89.0f;
        if (cam->pitch < -89.0f) cam->pitch = -89.0f;

        vec3 front;
        front[0] = cosf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
        front[1] = sinf(glm_rad(cam->pitch));
        front[2] = sinf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
        glm_vec3_normalize_to(front, cam->front);
        printf("Camera Yaw: %.2f, Pitch: %.2f, Front: [%.2f, %.2f, %.2f]\n", cam->yaw, cam->pitch, cam->front[0], cam->front[1], cam->front[2]);
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    init_vulkan(window, &vkCtx.instance, &vkCtx.physicalDevice, &vkCtx.device, &vkCtx.graphicsQueue, &vkCtx.surface, 
                &vkCtx.swapchain, &vkCtx.imageCount, &vkCtx.swapchainImages, &vkCtx.swapchainImageViews, &vkCtx.graphicsQueueFamilyIndex);
    printf("Vulkan initialized with queue family index: %u\n", vkCtx.graphicsQueueFamilyIndex);

    VkImageCreateInfo depthImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
    depthImageInfo.extent.width = WIDTH;
    depthImageInfo.extent.height = HEIGHT;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo depthAllocInfo = {};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(allocator, &depthImageInfo, &depthAllocInfo, &vkCtx.depthImage, &vkCtx.depthAllocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create depth image with VMA\n");
        exit(1);
    }
    printf("Depth image created with VMA\n");

    VkImageViewCreateInfo depthViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    depthViewInfo.image = vkCtx.depthImage;
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vkCtx.device, &depthViewInfo, NULL, &vkCtx.depthImageView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create depth image view\n");
        exit(1);
    }
    printf("Depth image view created\n");

    VkAttachmentDescription attachments[2] = {};
    attachments[0].format = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].format = VK_FORMAT_D32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthAttachmentRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(vkCtx.device, &renderPassInfo, NULL, &vkCtx.renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        exit(1);
    }
    printf("Render pass created\n");

    vkCtx.swapchainFramebuffers = malloc(vkCtx.imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkImageView attachments[] = {vkCtx.swapchainImageViews[i], vkCtx.depthImageView};
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx.renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = WIDTH;
        framebufferInfo.height = HEIGHT;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx.device, &framebufferInfo, NULL, &vkCtx.swapchainFramebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer %u\n", i);
            exit(1);
        }
    }
    printf("Framebuffers created\n");

    VkCommandPoolCreateInfo commandPoolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolInfo.queueFamilyIndex = vkCtx.graphicsQueueFamilyIndex;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vkCtx.device, &commandPoolInfo, NULL, &vkCtx.commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool with queue family index %u\n", vkCtx.graphicsQueueFamilyIndex);
        exit(1);
    }
    printf("Command pool created\n");

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(vkCtx.device, &allocInfo, &vkCtx.commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffer\n");
        exit(1);
    }
    printf("Command buffer allocated\n");

    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vkCtx.device, &fenceInfo, NULL, &vkCtx.inFlightFence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create synchronization objects\n");
        exit(1);
    }
    printf("Synchronization objects created\n");

    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    if (vkCreateSampler(vkCtx.device, &samplerInfo, NULL, &vkCtx.textureSampler) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture sampler\n");
        exit(1);
    }
    printf("Texture sampler created\n");

    VkDescriptorSetLayoutBinding layoutBindings[2] = {};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = layoutBindings;

    if (vkCreateDescriptorSetLayout(vkCtx.device, &layoutInfo, NULL, &vkCtx.descriptorSetLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        exit(1);
    }
    printf("Descriptor set layout created\n");

    VkBufferCreateInfo uniformBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    uniformBufferInfo.size = sizeof(UBO);
    uniformBufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uniformBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo uniformAllocInfo = {};
    uniformAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(allocator, &uniformBufferInfo, &uniformAllocInfo, &vkCtx.uniformBuffer, &vkCtx.uniformAllocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create uniform buffer with VMA\n");
        exit(1);
    }
    printf("Uniform buffer created with VMA\n");

    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolInfo.poolSizeCount = 2;
    descriptorPoolInfo.pPoolSizes = poolSizes;
    descriptorPoolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(vkCtx.device, &descriptorPoolInfo, NULL, &vkCtx.descriptorPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor pool\n");
        exit(1);
    }
    printf("Descriptor pool created\n");

    VkDescriptorSetAllocateInfo allocSetInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocSetInfo.descriptorPool = vkCtx.descriptorPool;
    allocSetInfo.descriptorSetCount = 1;
    allocSetInfo.pSetLayouts = &vkCtx.descriptorSetLayout;

    if (vkAllocateDescriptorSets(vkCtx.device, &allocSetInfo, &vkCtx.descriptorSet) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate descriptor set\n");
        exit(1);
    }
    printf("Descriptor set allocated\n");

    VkDescriptorBufferInfo bufferDescriptorInfo = {};
    bufferDescriptorInfo.buffer = vkCtx.uniformBuffer;
    bufferDescriptorInfo.offset = 0;
    bufferDescriptorInfo.range = sizeof(UBO);

    VkWriteDescriptorSet descriptorWrites[2] = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = vkCtx.descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferDescriptorInfo;

    VkDescriptorImageInfo textureInfo = {};
    textureInfo.sampler = vkCtx.textureSampler;
    textureInfo.imageView = VK_NULL_HANDLE; // Updated in create_text
    textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = vkCtx.descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &textureInfo;

    vkUpdateDescriptorSets(vkCtx.device, 1, &descriptorWrites[0], 0, NULL); // UBO initially
    printf("Descriptor set updated\n");

    create_pipeline();
    create_triangle();

    Camera cam = {{0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f};
    bool mouseCaptured = false;
    bool running = true;
    bool rotateObjects = false;
    float rotationAngle = 0.0f;
    Uint32 lastTime = SDL_GetTicks();
    SDL_Event event;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            update_camera(&cam, &event, &mouseCaptured, window, deltaTime);

            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_TAB: rotateObjects = !rotateObjects; printf("Object rotation %s\n", rotateObjects ? "enabled" : "disabled"); break;
                    case SDLK_1: rotationAngle = 0.0f; printf("Object rotation reset to 0\n"); break;
                    case SDLK_2: reset_camera(&cam); break;
                    case SDLK_4: vkCtx.triangle.exists ? destroy_triangle() : create_triangle(); break;
                    case SDLK_5: vkCtx.cube.exists ? destroy_cube() : create_cube(); break;
                    case SDLK_6: vkCtx.text.exists ? destroy_text() : create_text(); break;
                }
            }
        }

        if (rotateObjects) {
            rotationAngle += 90.0f * (deltaTime / 1000.0f);
            if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        }

        update_uniform_buffer(&cam, rotationAngle);

        vkWaitForFences(vkCtx.device, 1, &vkCtx.inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vkCtx.device, 1, &vkCtx.inFlightFence);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkCtx.device, vkCtx.swapchain, UINT64_MAX, vkCtx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to acquire next image: %d\n", result);
            exit(1);
        }

        vkResetCommandBuffer(vkCtx.commandBuffer, 0);
        record_command_buffer(imageIndex);

        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkSemaphore waitSemaphores[] = {vkCtx.imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCtx.commandBuffer;
        VkSemaphore signalSemaphores[] = {vkCtx.renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, vkCtx.inFlightFence) != VK_SUCCESS) {
            fprintf(stderr, "Failed to submit draw command buffer\n");
            exit(1);
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkCtx.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        if (vkQueuePresentKHR(vkCtx.graphicsQueue, &presentInfo) != VK_SUCCESS) {
            fprintf(stderr, "Failed to present image\n");
            exit(1);
        }
    }

    vkDeviceWaitIdle(vkCtx.device);
    if (vkCtx.triangle.exists) destroy_triangle();
    if (vkCtx.cube.exists) destroy_cube();
    if (vkCtx.text.exists) destroy_text();
    vkDestroySemaphore(vkCtx.device, vkCtx.renderFinishedSemaphore, NULL);
    vkDestroySemaphore(vkCtx.device, vkCtx.imageAvailableSemaphore, NULL);
    vkDestroyFence(vkCtx.device, vkCtx.inFlightFence, NULL);
    vkDestroyCommandPool(vkCtx.device, vkCtx.commandPool, NULL);
    vkDestroyPipeline(vkCtx.device, vkCtx.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(vkCtx.device, vkCtx.pipelineLayout, NULL);
    vkDestroyDescriptorPool(vkCtx.device, vkCtx.descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(vkCtx.device, vkCtx.descriptorSetLayout, NULL);
    vkDestroySampler(vkCtx.device, vkCtx.textureSampler, NULL);
    vmaDestroyBuffer(allocator, vkCtx.uniformBuffer, vkCtx.uniformAllocation);
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        vkDestroyFramebuffer(vkCtx.device, vkCtx.swapchainFramebuffers[i], NULL);
        vkDestroyImageView(vkCtx.device, vkCtx.swapchainImageViews[i], NULL);
    }
    free(vkCtx.swapchainFramebuffers);
    free(vkCtx.swapchainImageViews);
    vkDestroyImageView(vkCtx.device, vkCtx.depthImageView, NULL);
    vmaDestroyImage(allocator, vkCtx.depthImage, vkCtx.depthAllocation);
    vkDestroyRenderPass(vkCtx.device, vkCtx.renderPass, NULL);
    vkDestroySwapchainKHR(vkCtx.device, vkCtx.swapchain, NULL);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(vkCtx.device, NULL);
    vkDestroySurfaceKHR(vkCtx.instance, vkCtx.surface, NULL);
    vkDestroyInstance(vkCtx.instance, NULL);
    if (ft_library) FT_Done_FreeType(ft_library);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}