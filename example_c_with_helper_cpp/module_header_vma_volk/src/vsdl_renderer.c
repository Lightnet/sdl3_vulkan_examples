#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_log.h>
#include "vsdl_renderer.h"
#include "vsdl_types.h"
#include "vsdl_pipeline.h"

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

    VkSurfaceFormatKHR result = formats[0];
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            result = formats[i];
            break;
        }
    }
    free(formats);
    return result;
}

static VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            result = presentModes[i];
            break;
        }
    }
    free(presentModes);
    return result;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR* capabilities, SDL_Window* window) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    VkExtent2D extent = {(uint32_t)width, (uint32_t)height};
    extent.width = (extent.width < capabilities->minImageExtent.width) ? capabilities->minImageExtent.width : 
                   (extent.width > capabilities->maxImageExtent.width) ? capabilities->maxImageExtent.width : extent.width;
    extent.height = (extent.height < capabilities->minImageExtent.height) ? capabilities->minImageExtent.height : 
                    (extent.height > capabilities->maxImageExtent.height) ? capabilities->maxImageExtent.height : extent.height;
    return extent;
}

int vsdl_render_loop(VSDL_Context* ctx) {
    SDL_Log("Starting render loop");

    if (!SDL_Vulkan_CreateSurface(ctx->window, ctx->instance, NULL, &ctx->surface)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan surface: %s", SDL_GetError());
        return 0;
    }

    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = ctx->graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create command pool");
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        return 0;
    }
    SDL_Log("Command pool created");

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(ctx->physicalDevice, ctx->surface);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(ctx->physicalDevice, ctx->surface);
    VkExtent2D extent = chooseSwapExtent(&capabilities, ctx->window);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = ctx->surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(ctx->device, &swapchainInfo, NULL, &ctx->swapchain) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swapchain");
        vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        return 0;
    }
    SDL_Log("Swapchain created");

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &swapchainImageCount, NULL);
    ctx->swapchainImages = (VkImage*)malloc(swapchainImageCount * sizeof(VkImage));
    ctx->swapchainImageCount = swapchainImageCount;
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &swapchainImageCount, ctx->swapchainImages);
    ctx->swapchainImageFormat = surfaceFormat.format;
    ctx->swapchainExtent = extent;

    ctx->swapchainImageViews = (VkImageView*)malloc(swapchainImageCount * sizeof(VkImageView));
    ctx->swapchainImageViewCount = swapchainImageCount;
    for (size_t i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = ctx->swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = ctx->swapchainImageFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->swapchainImageViews[i]) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create image view %zu", i);
            return 0;
        }
    }
    SDL_Log("Swapchain image views created (count: %u)", swapchainImageCount);

    if (!create_pipeline(ctx)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create pipeline");
        return 0;
    }

    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(ctx->device, &fenceInfo, NULL, &ctx->frameFence) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create synchronization objects");
        return 0;
    }
    SDL_Log("Synchronization objects created");

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = ctx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(ctx->device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate command buffer");
        return 0;
    }

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            }
        }

        vkWaitForFences(ctx->device, 1, &ctx->frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx->device, 1, &ctx->frameFence);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, ctx->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire swapchain image");
            running = 0;
            continue;
        }

        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = ctx->renderPass;
        renderPassInfo.framebuffer = ctx->framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
        renderPassInfo.renderArea.extent = ctx->swapchainExtent;
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphicsPipeline);
        VkBuffer vertexBuffers[] = {ctx->vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &ctx->imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &ctx->renderFinishedSemaphore;

        if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->frameFence) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to submit draw command buffer");
            running = 0;
            continue;
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &ctx->renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &ctx->swapchain;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(ctx->graphicsQueue, &presentInfo);
    }

    SDL_Log("Render loop ended");
    vkQueueWaitIdle(ctx->graphicsQueue);

    vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &commandBuffer);
    if (ctx->frameFence) {
        SDL_Log("Destroying frame fence");
        vkDestroyFence(ctx->device, ctx->frameFence, NULL);
        ctx->frameFence = VK_NULL_HANDLE;
    }
    if (ctx->renderFinishedSemaphore) {
        SDL_Log("Destroying render finished semaphore");
        vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphore, NULL);
        ctx->renderFinishedSemaphore = VK_NULL_HANDLE;
    }
    if (ctx->imageAvailableSemaphore) {
        SDL_Log("Destroying image available semaphore");
        vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphore, NULL);
        ctx->imageAvailableSemaphore = VK_NULL_HANDLE;
    }

    free(ctx->swapchainImages);
    ctx->swapchainImages = NULL;
    ctx->swapchainImageCount = 0;

    return 1;
}