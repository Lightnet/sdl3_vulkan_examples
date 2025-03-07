#include "vsdl_cleanup.h"
#include <SDL3/SDL_log.h>

void vsdl_cleanup(VSDL_Context& ctx) {
    if (ctx.device) {
        vkDeviceWaitIdle(ctx.device);

        if (ctx.inFlightFence) vkDestroyFence(ctx.device, ctx.inFlightFence, nullptr);
        if (ctx.renderFinishedSemaphore) vkDestroySemaphore(ctx.device, ctx.renderFinishedSemaphore, nullptr);
        if (ctx.imageAvailableSemaphore) vkDestroySemaphore(ctx.device, ctx.imageAvailableSemaphore, nullptr);
        if (ctx.commandPool) {
            vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &ctx.commandBuffer);
            vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
        }
        for (auto framebuffer : ctx.framebuffers) {
            vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
        }
        if (ctx.graphicsPipeline) vkDestroyPipeline(ctx.device, ctx.graphicsPipeline, nullptr);
        if (ctx.pipelineLayout) vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayout, nullptr);
        if (ctx.renderPass) vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
        for (auto imageView : ctx.swapchainImageViews) {
            vkDestroyImageView(ctx.device, imageView, nullptr);
        }
        if (ctx.swapchain) vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
        
        // Destroy VMA allocator before device
        // if (ctx.allocator) {
        //     vmaDestroyAllocator(ctx.allocator);
        //     SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "VMA allocator destroyed");
        // }

        vkDestroyDevice(ctx.device, nullptr);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Vulkan device destroyed");
    }
    if (ctx.surface) {
        vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    }
    if (ctx.instance) {
        vkDestroyInstance(ctx.instance, nullptr);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Vulkan instance destroyed");
    }
    if (ctx.window) {
        SDL_DestroyWindow(ctx.window);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL window destroyed");
    }
    SDL_Quit();
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "SDL quit");
}