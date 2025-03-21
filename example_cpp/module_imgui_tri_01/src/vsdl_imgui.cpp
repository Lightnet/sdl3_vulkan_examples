#include "vsdl_imgui.h"
#include <SDL3/SDL_log.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

namespace vsdl {
    bool init_imgui(VSDL_Context& ctx) {
        // Create ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark(); // Optional: set a default style

        // Initialize SDL3 backend
        if (!ImGui_ImplSDL3_InitForVulkan(ctx.window)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize ImGui SDL3 backend");
            return false;
        }

        // Create descriptor pool for ImGui
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
        poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &ctx.imguiDescriptorPool) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create ImGui descriptor pool");
            return false;
        }

        // Initialize Vulkan backend
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = ctx.instance;
        initInfo.PhysicalDevice = ctx.physicalDevice;
        initInfo.Device = ctx.device;
        initInfo.QueueFamily = ctx.graphicsQueueFamilyIndex; // Use the stored index
        initInfo.Queue = ctx.graphicsQueue;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = ctx.imguiDescriptorPool;
        initInfo.RenderPass = ctx.renderPass;
        initInfo.Allocator = nullptr;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = static_cast<uint32_t>(ctx.swapchainImages.size());
        initInfo.CheckVkResultFn = nullptr;

        if (!ImGui_ImplVulkan_Init(&initInfo)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize ImGui Vulkan backend");
            return false;
        }

        // Upload fonts
        VkCommandPoolCreateInfo tempPoolInfo = {};
        tempPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        tempPoolInfo.queueFamilyIndex = ctx.graphicsQueueFamilyIndex; // Use the stored index
        tempPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        VkCommandPool tempCommandPool;
        if (vkCreateCommandPool(ctx.device, &tempPoolInfo, nullptr, &tempCommandPool) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create temporary command pool for ImGui");
            return false;
        }

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = tempCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VkCommandBuffer fontCmdBuffer;
        vkAllocateCommandBuffers(ctx.device, &allocInfo, &fontCmdBuffer);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(fontCmdBuffer, &beginInfo);

        ImGui_ImplVulkan_CreateFontsTexture();

        vkEndCommandBuffer(fontCmdBuffer);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &fontCmdBuffer;
        vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(ctx.graphicsQueue);

        vkFreeCommandBuffers(ctx.device, tempCommandPool, 1, &fontCmdBuffer);
        vkDestroyCommandPool(ctx.device, tempCommandPool, nullptr);

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ImGui initialized successfully");
        return true;
    }

    void imgui_new_frame(VSDL_Context& ctx, SDL_Event& event) {
        ImGui_ImplSDL3_ProcessEvent(&event); // Only process events here
    }

    void imgui_render(VSDL_Context& ctx, VkCommandBuffer commandBuffer) {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void shutdown_imgui(VSDL_Context& ctx) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        if (ctx.imguiDescriptorPool) {
            vkDestroyDescriptorPool(ctx.device, ctx.imguiDescriptorPool, nullptr);
            ctx.imguiDescriptorPool = VK_NULL_HANDLE;
        }

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "ImGui shutdown complete");
    }
}