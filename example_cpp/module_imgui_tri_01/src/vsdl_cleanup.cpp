#include "vsdl_cleanup.h"
#include "vsdl_imgui.h"
#include <SDL3/SDL_log.h>

void vsdl_cleanup(VSDL_Context& ctx) {
    if (ctx.device) {
        vkDeviceWaitIdle(ctx.device);

        vsdl::shutdown_imgui(ctx);

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

        vkDestroyDevice(ctx.device, nullptr);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Vulkan device destroyed");
    }

#if VSDL_ENABLE_VALIDATION_LAYERS
    if (ctx.instance && ctx.debugMessenger) {
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx.instance, "vkDestroyDebugUtilsMessengerEXT");
        if (vkDestroyDebugUtilsMessengerEXT) {
            vkDestroyDebugUtilsMessengerEXT(ctx.instance, ctx.debugMessenger, nullptr);
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Debug messenger destroyed");
        }
    }
#endif

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