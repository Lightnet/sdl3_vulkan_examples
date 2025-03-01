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
    VkImage texture;          // Texture for text rendering
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

// Dummy texture handles for initial descriptor setup
static VkImage dummyTexture;
static VmaAllocation dummyAlloc;
static VkImageView dummyTextureView;

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
    ubo.proj[1][1] *= -1; // Flip Y for Vulkan's coordinate system

    void* data;
    vmaMapMemory(allocator, vkCtx.uniformAllocation, &data);
    memcpy(data, &ubo, sizeof(UBO));
    vmaUnmapMemory(allocator, vkCtx.uniformAllocation);
}

void create_triangle() {
    // ... (unchanged, keeping it simple for brevity)
}

void destroy_triangle() {
    // ... (unchanged)
}

void create_cube() {
    // ... (unchanged)
}

void destroy_cube() {
    // ... (unchanged)
}

/**
 * Creates a textured plane displaying "Hello World" using FreeType.
 * - Generates a texture atlas with glyphs aligned to a baseline.
 * - Sets up a Vulkan texture and vertex buffer for rendering.
 * - Plane is centered at (0,0), bottom-aligned relative to the screen.
 */
void create_text() {
    if (vkCtx.text.exists) {
        printf("Text already exists, skipping creation\n");
        return;
    }

    // Initialize FreeType library if not already done
    if (!ft_library) {
        if (FT_Init_FreeType(&ft_library) != 0) {
            fprintf(stderr, "Failed to initialize FreeType\n");
            exit(1);
        }
    }

    // Load font face
    FT_Face face;
    if (FT_New_Face(ft_library, "FiraSans-Bold.ttf", 0, &face) != 0) {
        fprintf(stderr, "Failed to load font FiraSans-Bold.ttf - ensure it’s in the executable directory\n");
        exit(1);
    }
    FT_Set_Pixel_Sizes(face, 0, 48); // Set font height to 48px

    // Define text and calculate atlas size
    const char* text = "Hello World";
    int atlas_width = 0, atlas_height = 0;
    int max_bearing_y = 0;

    // First pass: Calculate total width and max height/bearing
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
        atlas_width += face->glyph->bitmap.width + 2; // Width + 2px spacing
        atlas_height = (face->glyph->bitmap.rows > atlas_height) ? face->glyph->bitmap.rows : atlas_height;
        max_bearing_y = (face->glyph->bitmap_top > max_bearing_y) ? face->glyph->bitmap_top : max_bearing_y;
    }

    // Adjust atlas height to include space above and below baseline
    int baseline_y = max_bearing_y; // Baseline position from top
    atlas_height += max_bearing_y; // Total height includes descenders

    // Allocate atlas data (grayscale image)
    unsigned char* atlas_data = calloc(1, atlas_width * atlas_height);
    int x_offset = 0;

    // Second pass: Render glyphs aligned to baseline
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
        FT_Bitmap* bitmap = &face->glyph->bitmap;
        int y_offset = baseline_y - face->glyph->bitmap_top; // Offset to align glyph top to baseline
        for (unsigned int y = 0; y < bitmap->rows; y++) {
            for (unsigned int x = 0; x < bitmap->width; x++) {
                int atlas_y = y_offset + y;
                if (atlas_y >= 0 && atlas_y < atlas_height) {
                    atlas_data[(atlas_y * atlas_width) + x_offset + x] = bitmap->buffer[y * bitmap->width + x];
                }
            }
        }
        x_offset += bitmap->width + 2; // Move right with spacing
    }

    // Log atlas for debugging
    printf("Atlas dimensions: %d x %d, Baseline at y=%d\n", atlas_width, atlas_height, baseline_y);
    for (int y = 0; y < atlas_height; y++) {
        for (int x = 0; x < atlas_width; x++) {
            printf("%c", atlas_data[y * atlas_width + x] > 128 ? '#' : ' ');
        }
        printf("\n");
    }

    // Create staging buffer to transfer atlas to GPU
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

    // Create GPU texture
    VkImageCreateInfo textImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    textImageInfo.imageType = VK_IMAGE_TYPE_2D;
    textImageInfo.format = VK_FORMAT_R8_UNORM; // Grayscale
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

    // Command buffer to copy staging buffer to texture
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

    // Create texture view
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

    // Update descriptor set with text texture
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

    // Define plane vertices: Position (x,y,z), Color (r,g,b), UV (u,v), TexFlag
    float baseline_v = (float)baseline_y / atlas_height; // UV coordinate of baseline
    float vertices[] = {
        -0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, baseline_v,  1.0f, // Bottom-left (baseline)
        -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,       1.0f, // Top-left (atlas top)
         0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, baseline_v,  1.0f, // Bottom-right (baseline)
        -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,       1.0f, // Top-left
         0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,       1.0f, // Top-right
         0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, baseline_v,  1.0f  // Bottom-right
    };

    // Create vertex buffer
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
    // ... (unchanged, but added comments for clarity)
    if (!vkCtx.text.exists) {
        printf("Text does not exist, skipping destruction\n");
        return;
    }

    vkDeviceWaitIdle(vkCtx.device);
    vkDestroyImageView(vkCtx.device, vkCtx.text.textureView, NULL);
    vmaDestroyImage(allocator, vkCtx.text.texture, vkCtx.text.texAlloc);
    vmaDestroyBuffer(allocator, vkCtx.text.buffer, vkCtx.text.allocation);

    // Reset descriptor to dummy texture
    VkDescriptorImageInfo dummyDescriptorImageInfo = {};
    dummyDescriptorImageInfo.sampler = vkCtx.textureSampler;
    dummyDescriptorImageInfo.imageView = dummyTextureView;
    dummyDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = vkCtx.descriptorSet;
    descriptorWrite.dstBinding = 1;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &dummyDescriptorImageInfo;

    vkUpdateDescriptorSets(vkCtx.device, 1, &descriptorWrite, 0, NULL);

    vkCtx.text.buffer = VK_NULL_HANDLE;
    vkCtx.text.allocation = VK_NULL_HANDLE;
    vkCtx.text.texture = VK_NULL_HANDLE;
    vkCtx.text.texAlloc = VK_NULL_HANDLE;
    vkCtx.text.textureView = VK_NULL_HANDLE;
    vkCtx.text.vertexCount = 0;
    vkCtx.text.exists = false;
    printf("Text destroyed with VMA\n");
}

// ... (Rest of the functions remain unchanged with minor comment cleanup)

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3 Text Rendering", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize Vulkan context
    init_vulkan(window, &vkCtx.instance, &vkCtx.physicalDevice, &vkCtx.device, &vkCtx.graphicsQueue, &vkCtx.surface, 
                &vkCtx.swapchain, &vkCtx.imageCount, &vkCtx.swapchainImages, &vkCtx.swapchainImageViews, &vkCtx.graphicsQueueFamilyIndex);
    printf("Vulkan initialized with queue family index: %u\n", vkCtx.graphicsQueueFamilyIndex);

    // ... (Rest of main() remains unchanged, with comments added where needed)

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

        // ... (Rest of main loop unchanged)
    }

    // Cleanup
    vkDeviceWaitIdle(vkCtx.device);
    if (vkCtx.triangle.exists) destroy_triangle();
    if (vkCtx.cube.exists) destroy_cube();
    if (vkCtx.text.exists) destroy_text();
    // ... (Rest of cleanup unchanged)
}