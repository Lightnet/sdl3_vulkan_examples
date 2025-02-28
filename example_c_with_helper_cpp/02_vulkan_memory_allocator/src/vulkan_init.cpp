#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#define VMA_IMPLEMENTATION  // Define before including vk_mem_alloc.h to include implementation
#include <vk_mem_alloc.h>
#include <stdexcept>

extern "C" {
    VmaAllocator allocator;  // Global VMA allocator

    void init_vulkan(SDL_Window* window, VkInstance* instance, VkPhysicalDevice* physicalDevice, VkDevice* device, 
                     VkQueue* graphicsQueue, VkSurfaceKHR* surface, VkSwapchainKHR* swapchain, uint32_t* imageCount,
                     VkImage** swapchainImages, VkImageView** swapchainImageViews, uint32_t* graphicsQueueFamilyIndex) {
        vkb::InstanceBuilder builder;
        auto inst_ret = builder.set_app_name("Vulkan SDL3")
                              .request_validation_layers(false)
                              .build();
        if (!inst_ret) {
            fprintf(stderr, "Failed to create Vulkan instance: %s\n", inst_ret.error().message().c_str());
            exit(1);
        }
        *instance = inst_ret.value().instance;

        if (!SDL_Vulkan_CreateSurface(window, *instance, NULL, surface)) {
            fprintf(stderr, "Failed to create Vulkan surface: %s\n", SDL_GetError());
            exit(1);
        }

        vkb::PhysicalDeviceSelector selector{inst_ret.value()};
        auto phys_ret = selector.set_surface(*surface).select();
        if (!phys_ret) {
            fprintf(stderr, "Failed to select physical device: %s\n", phys_ret.error().message().c_str());
            exit(1);
        }
        *physicalDevice = phys_ret.value().physical_device;

        vkb::DeviceBuilder device_builder{phys_ret.value()};
        auto dev_ret = device_builder.build();
        if (!dev_ret) {
            fprintf(stderr, "Failed to create device: %s\n", dev_ret.error().message().c_str());
            exit(1);
        }
        *device = dev_ret.value().device;
        *graphicsQueue = dev_ret.value().get_queue(vkb::QueueType::graphics).value();
        *graphicsQueueFamilyIndex = dev_ret.value().get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = *physicalDevice;
        allocatorInfo.device = *device;
        allocatorInfo.instance = *instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create VMA allocator\n");
            exit(1);
        }

        vkb::SwapchainBuilder swapchain_builder{dev_ret.value()};
        auto swap_ret = swapchain_builder.set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                        .set_desired_extent(800, 600)
                                        .build();
        if (!swap_ret) {
            fprintf(stderr, "Failed to create swapchain: %s\n", swap_ret.error().message().c_str());
            exit(1);
        }
        *swapchain = swap_ret.value().swapchain;
        *imageCount = swap_ret.value().image_count;
        *swapchainImages = (VkImage*)malloc(*imageCount * sizeof(VkImage));
        memcpy(*swapchainImages, swap_ret.value().get_images().value().data(), *imageCount * sizeof(VkImage));
        *swapchainImageViews = (VkImageView*)malloc(*imageCount * sizeof(VkImageView));
        auto views = swap_ret.value().get_image_views().value();
        for (uint32_t i = 0; i < *imageCount; i++) (*swapchainImageViews)[i] = views[i];
    }
}