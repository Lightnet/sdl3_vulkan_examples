#include "vsdl_clean_up.h"
#include <stdlib.h>

void vsdl_clean_up(void) {
    LOG("Cleaning up VSDL");
    if (g_context.device != VK_NULL_HANDLE) {
        if (g_context.imageAvailableSemaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(g_context.device, g_context.imageAvailableSemaphore, NULL);
        if (g_context.renderFinishedSemaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(g_context.device, g_context.renderFinishedSemaphore, NULL);
        if (g_context.inFlightFence != VK_NULL_HANDLE)
            vkDestroyFence(g_context.device, g_context.inFlightFence, NULL);
        if (g_context.commandPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(g_context.device, g_context.commandPool, NULL);
        for (uint32_t i = 0; i < g_context.swapchainImageCount && g_context.framebuffers; i++)
            vkDestroyFramebuffer(g_context.device, g_context.framebuffers[i], NULL);
        if (g_context.graphicsPipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(g_context.device, g_context.graphicsPipeline, NULL);
        if (g_context.pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(g_context.device, g_context.pipelineLayout, NULL);
        if (g_context.renderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(g_context.device, g_context.renderPass, NULL);
        for (uint32_t i = 0; i < g_context.swapchainImageCount && g_context.swapchainImageViews; i++)
            vkDestroyImageView(g_context.device, g_context.swapchainImageViews[i], NULL);
        if (g_context.swapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(g_context.device, g_context.swapchain, NULL);
        vkDestroyDevice(g_context.device, NULL);
    }
    if (g_context.instance != VK_NULL_HANDLE && g_context.surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(g_context.instance, g_context.surface, NULL);
    if (g_context.instance != VK_NULL_HANDLE)
        vkDestroyInstance(g_context.instance, NULL);
    if (g_context.window)
        SDL_DestroyWindow(g_context.window);
    free(g_context.framebuffers);
    free(g_context.swapchainImageViews);
    free(g_context.swapchainImages);
    SDL_Quit();
    LOG("Cleanup completed");
}