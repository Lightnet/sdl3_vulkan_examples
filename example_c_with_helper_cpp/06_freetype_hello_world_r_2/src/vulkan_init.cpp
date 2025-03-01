// vulkan_init.cpp: Initializes Vulkan core components using vk-bootstrap and VMA
// Purpose: Sets up Vulkan instance, device, swapchain, and memory allocator for main.c

#include "vulkan_init.h"
#include <SDL3/SDL_vulkan.h>  // For SDL_Vulkan_CreateSurface
#include <VkBootstrap.h>      // vk-bootstrap for simplified Vulkan setup
#include <stdio.h>            // For error logging

// Define the global VMA allocator, declared in vulkan_init.h
VmaAllocator allocator;

// Initializes Vulkan with vk-bootstrap and VMA
// Note: Uses C linkage for compatibility with main.c
extern "C" void init_vulkan(SDL_Window* window, VkInstance* instance, VkPhysicalDevice* physicalDevice, VkDevice* device, 
                           VkQueue* graphicsQueue, VkSurfaceKHR* surface, VkSwapchainKHR* swapchain, uint32_t* imageCount,
                           VkImage** swapchainImages, VkImageView** swapchainImageViews, uint32_t* graphicsQueueFamilyIndex) {
    // Create Vulkan instance
    vkb::InstanceBuilder builder;
    builder.set_app_name("Vulkan SDL3")          // App name for Vulkan metadata
           .set_engine_name("Custom")            // Engine identifier
           .require_api_version(1, 3, 0);        // Ensure Vulkan 1.3 compatibility

#ifdef _DEBUG
    // Enable validation layers in Debug builds for runtime error checking
    builder.request_validation_layers()
           .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
#endif

    auto inst_ret = builder.build();
    if (!inst_ret) {
        fprintf(stderr, "Failed to create Vulkan instance: %s\n", inst_ret.error().message().c_str());
        exit(1);
    }
    *instance = inst_ret.value();

    // Create surface for rendering using SDL
    if (!SDL_Vulkan_CreateSurface(window, *instance, NULL, surface)) {
        fprintf(stderr, "Failed to create Vulkan surface: %s\n", SDL_GetError());
        exit(1);
    }

    // Select physical device (GPU)
    vkb::PhysicalDeviceSelector phys_selector{inst_ret.value()};
    phys_selector.set_surface(*surface)
                 .set_minimum_version(1, 3)                  // Require Vulkan 1.3
                 .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete); // Favor discrete GPUs
    auto phys_ret = phys_selector.select();
    if (!phys_ret) {
        fprintf(stderr, "Failed to select physical device: %s\n", phys_ret.error().message().c_str());
        exit(1);
    }
    *physicalDevice = phys_ret.value();

    // Create logical device
    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        fprintf(stderr, "Failed to create Vulkan device: %s\n", dev_ret.error().message().c_str());
        exit(1);
    }
    *device = dev_ret.value();

    // Get graphics queue for rendering
    auto queue_ret = dev_ret.value().get_queue(vkb::QueueType::graphics);
    if (!queue_ret) {
        fprintf(stderr, "Failed to get graphics queue: %s\n", queue_ret.error().message().c_str());
        exit(1);
    }
    *graphicsQueue = queue_ret.value();
    *graphicsQueueFamilyIndex = dev_ret.value().get_queue_index(vkb::QueueType::graphics).value();

    // Create swapchain for presenting rendered images
    vkb::SwapchainBuilder swapchain_builder{dev_ret.value()};
    swapchain_builder.set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}) // Standard color format
                     .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)                        // VSync
                     .set_desired_extent(800, 600);                                             // Hardcoded size (matches main.c’s WIDTH, HEIGHT)
    auto swap_ret = swapchain_builder.build();
    if (!swap_ret) {
        fprintf(stderr, "Failed to create swapchain: %s\n", swap_ret.error().message().c_str());
        exit(1);
    }
    *swapchain = swap_ret.value().swapchain;
    *imageCount = swap_ret.value().image_count;
    *swapchainImages = swap_ret.value().get_images().value().data(); // Vulkan-managed, don’t free manually

    // Allocate and copy swapchain image views (we own this memory)
    auto image_views = swap_ret.value().get_image_views().value();
    *swapchainImageViews = (VkImageView*)malloc(*imageCount * sizeof(VkImageView));
    if (!*swapchainImageViews) {
        fprintf(stderr, "Failed to allocate memory for swapchain image views\n");
        exit(1);
    }
    for (uint32_t i = 0; i < *imageCount; i++) {
        (*swapchainImageViews)[i] = image_views[i];
    }

    // Initialize VMA allocator for memory management
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = *physicalDevice;
    allocatorInfo.device = *device;
    allocatorInfo.instance = *instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create VMA allocator\n");
        exit(1);
    }
}

// Notes for Users:
// - vk-bootstrap simplifies Vulkan setup; see VkBootstrap.h for options (e.g., other present modes).
// - swapchainImages is Vulkan-owned; don’t free it (handled by vkDestroySwapchainKHR).
// - swapchainImageViews is malloc’d here; main.c must free it.
// - Hardcoded 800x600 extent; consider passing width/height from main.c for flexibility.
// - allocator must be destroyed with vmaDestroyAllocator before vkDestroyDevice in main.c.