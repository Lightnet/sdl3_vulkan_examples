// vsdl_cleanup.cpp
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include "vsdl_cleanup.h"
#include "vsdl_types.h"

void vsdl_cleanup(VSDL_Context& ctx) {
    SDL_Log("init cleanup");

    if (ctx.uniformBuffer) {
        SDL_Log("Destroying uniform buffer");
        vmaDestroyBuffer(ctx.allocator, ctx.uniformBuffer, ctx.uniformBufferAllocation);
        ctx.uniformBuffer = VK_NULL_HANDLE;
        ctx.uniformBufferAllocation = VK_NULL_HANDLE;
    }

    for (auto& mesh : ctx.meshes) {
        SDL_Log("Destroying mesh");
        vmaDestroyBuffer(ctx.allocator, mesh.vertexBuffer, mesh.vertexAllocation);
        if (mesh.indexBuffer) {
            vmaDestroyBuffer(ctx.allocator, mesh.indexBuffer, mesh.indexAllocation);
        }
    }
    ctx.meshes.clear();

    if (ctx.allocator) {
        SDL_Log("Destroying VMA allocator");
        vmaDestroyAllocator(ctx.allocator);
        ctx.allocator = VK_NULL_HANDLE;
    }

    if (ctx.commandPool) {
        SDL_Log("Destroying command pool");
        vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
        ctx.commandPool = VK_NULL_HANDLE;
    }

    for (auto& fb : ctx.framebuffers) {
        SDL_Log("Destroying framebuffer");
        vkDestroyFramebuffer(ctx.device, fb, nullptr);
    }
    ctx.framebuffers.clear();

    if (ctx.graphicsPipeline) {
        SDL_Log("Destroying graphics pipeline");
        vkDestroyPipeline(ctx.device, ctx.graphicsPipeline, nullptr);
        ctx.graphicsPipeline = VK_NULL_HANDLE;
    }

    if (ctx.pipelineLayout) {
        SDL_Log("Destroying pipeline layout");
        vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayout, nullptr);
        ctx.pipelineLayout = VK_NULL_HANDLE;
    }

    for (auto& view : ctx.swapchainImageViews) {
        SDL_Log("Destroying swapchain image view");
        vkDestroyImageView(ctx.device, view, nullptr);
    }
    ctx.swapchainImageViews.clear();

    if (ctx.swapchain) {
        SDL_Log("Destroying swapchain");
        vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
        ctx.swapchain = VK_NULL_HANDLE;
    }

    if (ctx.renderPass) {
        SDL_Log("Destroying render pass");
        vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
        ctx.renderPass = VK_NULL_HANDLE;
    }

    if (ctx.descriptorPool) {
        SDL_Log("Destroying descriptor pool");
        vkDestroyDescriptorPool(ctx.device, ctx.descriptorPool, nullptr);
        ctx.descriptorPool = VK_NULL_HANDLE;
    }

    if (ctx.descriptorSetLayout) {
        SDL_Log("Destroying descriptor set layout");
        vkDestroyDescriptorSetLayout(ctx.device, ctx.descriptorSetLayout, nullptr);
        ctx.descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (ctx.device) {
        SDL_Log("Destroying device");
        vkDestroyDevice(ctx.device, nullptr);
        ctx.device = VK_NULL_HANDLE;
    }

    if (ctx.surface) {
        SDL_Log("Destroying surface");
        vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
        ctx.surface = VK_NULL_HANDLE;
    }

    if (ctx.instance) {
        SDL_Log("Destroying instance");
        vkDestroyInstance(ctx.instance, nullptr);
        ctx.instance = VK_NULL_HANDLE;
    }

    if (ctx.window) {
        SDL_Log("Destroying window");
        SDL_DestroyWindow(ctx.window);
        ctx.window = nullptr;
    }

    SDL_Log("Quitting SDL");
    SDL_Quit();
}