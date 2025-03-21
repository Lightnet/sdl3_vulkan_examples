#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include "vsdl_cleanup.h"
#include "vsdl_types.h"

void vsdl_cleanup(VSDL_Context* ctx) {
    SDL_Log("init cleanup");

    if (ctx->allocator) {
        if (ctx->vertexBuffer) {
            SDL_Log("Destroying vertex buffer");
            vmaDestroyBuffer(ctx->allocator, ctx->vertexBuffer, ctx->vertexBufferAllocation);
            ctx->vertexBuffer = VK_NULL_HANDLE;
        }
        SDL_Log("Destroying VMA allocator");
        vmaDestroyAllocator(ctx->allocator);
        ctx->allocator = VK_NULL_HANDLE;
    }

    if (ctx->device) {
        if (ctx->commandPool) {
            SDL_Log("Destroying command pool");
            vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
            ctx->commandPool = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < ctx->framebufferCount; i++) {
            if (ctx->framebuffers[i]) {
                SDL_Log("Destroying framebuffer");
                vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
                ctx->framebuffers[i] = VK_NULL_HANDLE;
            }
        }
        free(ctx->framebuffers);
        ctx->framebuffers = NULL;
        ctx->framebufferCount = 0;

        if (ctx->graphicsPipeline) {
            SDL_Log("Destroying graphics pipeline");
            vkDestroyPipeline(ctx->device, ctx->graphicsPipeline, NULL);
            ctx->graphicsPipeline = VK_NULL_HANDLE;
        }

        if (ctx->pipelineLayout) {
            SDL_Log("Destroying pipeline layout");
            vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, NULL);
            ctx->pipelineLayout = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < ctx->swapchainImageViewCount; i++) {
            if (ctx->swapchainImageViews[i]) {
                SDL_Log("Destroying swapchain image view");
                vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], NULL);
                ctx->swapchainImageViews[i] = VK_NULL_HANDLE;
            }
        }
        free(ctx->swapchainImageViews);
        ctx->swapchainImageViews = NULL;
        ctx->swapchainImageViewCount = 0;

        if (ctx->swapchain) {
            SDL_Log("Destroying swapchain");
            vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
            ctx->swapchain = VK_NULL_HANDLE;
        }

        if (ctx->renderPass) {
            SDL_Log("Destroying render pass");
            vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
            ctx->renderPass = VK_NULL_HANDLE;
        }

        SDL_Log("Destroying device");
        vkDestroyDevice(ctx->device, NULL);
        ctx->device = VK_NULL_HANDLE;
    }

    if (ctx->surface) {
        SDL_Log("Destroying surface");
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        ctx->surface = VK_NULL_HANDLE;
    }

    if (ctx->instance) {
        SDL_Log("Destroying instance");
        vkDestroyInstance(ctx->instance, NULL);
        ctx->instance = VK_NULL_HANDLE;
    }

    if (ctx->window) {
        SDL_Log("Destroying window");
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }

    SDL_Log("Quitting SDL");
    SDL_Quit();
}