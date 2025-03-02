#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "vsdl_types.h"
#include "vsdl_vulkan_init.h"
#include "vsdl_camera.h"
#include "vsdl_render.h"
#include "vsdl_mesh.h"
#include "vsdl_log.h"

#define WIDTH 800
#define HEIGHT 600

static VulkanContext vkCtx = {0};
static VkImage dummyTexture;
static VmaAllocation dummyAlloc;
VkImageView dummyTextureView;

int main(int argc, char* argv[]) {
    vsdl_init_log("debug.log", true);
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3 Text Rendering", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if (!window) {
        vsdl_log("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    init_vulkan(window, &vkCtx.instance, &vkCtx.physicalDevice, &vkCtx.device, &vkCtx.graphicsQueue, &vkCtx.surface,
                &vkCtx.swapchain, &vkCtx.imageCount, &vkCtx.swapchainImages, &vkCtx.swapchainImageViews, &vkCtx.graphicsQueueFamilyIndex);

    // Create descriptor set layout
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
        vsdl_log("Failed to create descriptor set layout\n");
        exit(1);
    }

    // Create render pass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(vkCtx.device, &renderPassInfo, NULL, &vkCtx.renderPass) != VK_SUCCESS) {
        vsdl_log("Failed to create render pass\n");
        exit(1);
    }

    // Create framebuffers
    vkCtx.swapchainFramebuffers = malloc(vkCtx.imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkCtx.swapchainImageViews[i];
        framebufferInfo.width = WIDTH;
        framebufferInfo.height = HEIGHT;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx.device, &framebufferInfo, NULL, &vkCtx.swapchainFramebuffers[i]) != VK_SUCCESS) {
            vsdl_log("Failed to create framebuffer %u\n", i);
            exit(1);
        }
    }

    // Create command pool and buffer
    VkCommandPoolCreateInfo commandPoolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolInfo.queueFamilyIndex = vkCtx.graphicsQueueFamilyIndex;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vkCtx.device, &commandPoolInfo, NULL, &vkCtx.commandPool) != VK_SUCCESS) {
        vsdl_log("Failed to create command pool\n");
        exit(1);
    }

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(vkCtx.device, &allocInfo, &vkCtx.commandBuffer) != VK_SUCCESS) {
        vsdl_log("Failed to allocate command buffer\n");
        exit(1);
    }

    // Create synchronization objects
    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vkCtx.device, &fenceInfo, NULL, &vkCtx.inFlightFence) != VK_SUCCESS) {
        vsdl_log("Failed to create synchronization objects\n");
        exit(1);
    }

    // Create uniform buffer
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(mat4) * 3;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfoBuf = {};
    allocInfoBuf.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfoBuf, &vkCtx.uniformBuffer, &vkCtx.uniformAllocation, NULL) != VK_SUCCESS) {
        vsdl_log("Failed to create uniform buffer\n");
        exit(1);
    }

    // Create dummy texture
    VkImageCreateInfo dummyImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    dummyImageInfo.imageType = VK_IMAGE_TYPE_2D;
    dummyImageInfo.format = VK_FORMAT_R8_UNORM;
    dummyImageInfo.extent.width = 1;
    dummyImageInfo.extent.height = 1;
    dummyImageInfo.extent.depth = 1;
    dummyImageInfo.mipLevels = 1;
    dummyImageInfo.arrayLayers = 1;
    dummyImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    dummyImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    dummyImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    dummyImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    dummyImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo dummyAllocInfo = {};
    dummyAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (vmaCreateImage(allocator, &dummyImageInfo, &dummyAllocInfo, &dummyTexture, &dummyAlloc, NULL) != VK_SUCCESS) {
        vsdl_log("Failed to create dummy texture\n");
        exit(1);
    }

    VkImageViewCreateInfo dummyViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    dummyViewInfo.image = dummyTexture;
    dummyViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    dummyViewInfo.format = VK_FORMAT_R8_UNORM;
    dummyViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    dummyViewInfo.subresourceRange.baseMipLevel = 0;
    dummyViewInfo.subresourceRange.levelCount = 1;
    dummyViewInfo.subresourceRange.baseArrayLayer = 0;
    dummyViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vkCtx.device, &dummyViewInfo, NULL, &dummyTextureView) != VK_SUCCESS) {
        vsdl_log("Failed to create dummy texture view\n");
        exit(1);
    }

    // Transition dummy texture to shader-readable layout
    VkCommandBuffer transitionCmd;
    VkCommandBufferAllocateInfo transitionAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    transitionAllocInfo.commandPool = vkCtx.commandPool;
    transitionAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    transitionAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkCtx.device, &transitionAllocInfo, &transitionCmd);

    VkCommandBufferBeginInfo transitionBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    transitionBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(transitionCmd, &transitionBeginInfo);

    VkImageMemoryBarrier dummyBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    dummyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    dummyBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dummyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dummyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    dummyBarrier.image = dummyTexture;
    dummyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    dummyBarrier.subresourceRange.baseMipLevel = 0;
    dummyBarrier.subresourceRange.levelCount = 1;
    dummyBarrier.subresourceRange.baseArrayLayer = 0;
    dummyBarrier.subresourceRange.layerCount = 1;
    dummyBarrier.srcAccessMask = 0;
    dummyBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(transitionCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &dummyBarrier);
    vkEndCommandBuffer(transitionCmd);

    VkSubmitInfo transitionSubmit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    transitionSubmit.commandBufferCount = 1;
    transitionSubmit.pCommandBuffers = &transitionCmd;
    vkQueueSubmit(vkCtx.graphicsQueue, 1, &transitionSubmit, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkCtx.graphicsQueue);
    vkFreeCommandBuffers(vkCtx.device, vkCtx.commandPool, 1, &transitionCmd);

    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(vkCtx.device, &poolInfo, NULL, &vkCtx.descriptorPool) != VK_SUCCESS) {
        vsdl_log("Failed to create descriptor pool\n");
        exit(1);
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocSetInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocSetInfo.descriptorPool = vkCtx.descriptorPool;
    allocSetInfo.descriptorSetCount = 1;
    allocSetInfo.pSetLayouts = &vkCtx.descriptorSetLayout;
    if (vkAllocateDescriptorSets(vkCtx.device, &allocSetInfo, &vkCtx.descriptorSet) != VK_SUCCESS) {
        vsdl_log("Failed to allocate descriptor set\n");
        exit(1);
    }

    // Create texture sampler
    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(vkCtx.device, &samplerInfo, NULL, &vkCtx.textureSampler) != VK_SUCCESS) {
        vsdl_log("Failed to create texture sampler\n");
        exit(1);
    }

    // Update descriptor set with uniform buffer and dummy texture
    VkDescriptorBufferInfo bufferDescriptorInfo = {};
    bufferDescriptorInfo.buffer = vkCtx.uniformBuffer;
    bufferDescriptorInfo.offset = 0;
    bufferDescriptorInfo.range = sizeof(mat4) * 3;

    VkDescriptorImageInfo dummyDescriptorImageInfo = {};
    dummyDescriptorImageInfo.sampler = vkCtx.textureSampler;
    dummyDescriptorImageInfo.imageView = dummyTextureView; // Now valid
    dummyDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrites[2] = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = vkCtx.descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferDescriptorInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = vkCtx.descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &dummyDescriptorImageInfo;

    vkUpdateDescriptorSets(vkCtx.device, 2, descriptorWrites, 0, NULL);

    vsdl_create_pipeline(&vkCtx);
    vsdl_create_triangle(&vkCtx, &vkCtx.triangle);

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
            vsdl_update_camera(&cam, &event, &mouseCaptured, window, deltaTime);

            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_TAB: rotateObjects = !rotateObjects; vsdl_log("Object rotation %s\n", rotateObjects ? "enabled" : "disabled"); break;
                    case SDLK_1: rotationAngle = 0.0f; vsdl_log("Object rotation reset to 0\n"); break;
                    case SDLK_2: vsdl_reset_camera(&cam); break;
                    case SDLK_4: vkCtx.triangle.exists ? vsdl_destroy_triangle(&vkCtx, &vkCtx.triangle) : vsdl_create_triangle(&vkCtx, &vkCtx.triangle); break;
                    case SDLK_5: vkCtx.cube.exists ? vsdl_destroy_cube(&vkCtx, &vkCtx.cube) : vsdl_create_cube(&vkCtx, &vkCtx.cube); break;
                    case SDLK_6: vkCtx.text.exists ? vsdl_destroy_text(&vkCtx, &vkCtx.text) : vsdl_create_text(&vkCtx, &vkCtx.text); break;
                }
            }
        }

        if (rotateObjects) {
            rotationAngle += 90.0f * (deltaTime / 1000.0f);
            if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        }

        vsdl_update_uniform_buffer(&vkCtx, &cam, rotationAngle);

        vkWaitForFences(vkCtx.device, 1, &vkCtx.inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vkCtx.device, 1, &vkCtx.inFlightFence);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkCtx.device, vkCtx.swapchain, UINT64_MAX, vkCtx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            vsdl_log("Failed to acquire next image: %d\n", result);
            return 1;
        }

        vkResetCommandBuffer(vkCtx.commandBuffer, 0);
        vsdl_record_command_buffer(&vkCtx, imageIndex);

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
            vsdl_log("Failed to submit draw command buffer\n");
            return 1;
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkCtx.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        if (vkQueuePresentKHR(vkCtx.graphicsQueue, &presentInfo) != VK_SUCCESS) {
            vsdl_log("Failed to present image\n");
            return 1;
        }
    }

    vkDeviceWaitIdle(vkCtx.device);
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        vkDestroyFramebuffer(vkCtx.device, vkCtx.swapchainFramebuffers[i], NULL);
    }
    free(vkCtx.swapchainFramebuffers);
    vkDestroyRenderPass(vkCtx.device, vkCtx.renderPass, NULL);
    vkDestroyCommandPool(vkCtx.device, vkCtx.commandPool, NULL);
    vkDestroySemaphore(vkCtx.device, vkCtx.imageAvailableSemaphore, NULL);
    vkDestroySemaphore(vkCtx.device, vkCtx.renderFinishedSemaphore, NULL);
    vkDestroyFence(vkCtx.device, vkCtx.inFlightFence, NULL);
    vkDestroyDescriptorPool(vkCtx.device, vkCtx.descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(vkCtx.device, vkCtx.descriptorSetLayout, NULL);
    vkDestroySampler(vkCtx.device, vkCtx.textureSampler, NULL);
    vkDestroyImageView(vkCtx.device, dummyTextureView, NULL);
    vmaDestroyImage(allocator, dummyTexture, dummyAlloc);
    vmaDestroyBuffer(allocator, vkCtx.uniformBuffer, vkCtx.uniformAllocation);
    vsdl_cleanup_log();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}