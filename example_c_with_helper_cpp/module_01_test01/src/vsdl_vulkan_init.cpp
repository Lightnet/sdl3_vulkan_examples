// vsdl_vulkan_init.cpp
#include "vsdl_vulkan_init.h" // Corrected include
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <stdio.h>

VmaAllocator allocator;

extern "C" void init_vulkan(SDL_Window* window, VkInstance* instance, VkPhysicalDevice* physicalDevice, VkDevice* device, 
                           VkQueue* graphicsQueue, VkSurfaceKHR* surface, VkSwapchainKHR* swapchain, uint32_t* imageCount,
                           VkImage** swapchainImages, VkImageView** swapchainImageViews, uint32_t* graphicsQueueFamilyIndex) {
    vkb::InstanceBuilder builder;
    builder.set_app_name("Vulkan SDL3")
           .set_engine_name("Custom")
           .require_api_version(1, 3, 0);
#ifdef _DEBUG
    builder.request_validation_layers()
           .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
#endif
    auto inst_ret = builder.build();
    if (!inst_ret) {
        fprintf(stderr, "Failed to create Vulkan instance: %s\n", inst_ret.error().message().c_str());
        exit(1);
    }
    *instance = inst_ret.value();

    if (!SDL_Vulkan_CreateSurface(window, *instance, NULL, surface)) {
        fprintf(stderr, "Failed to create Vulkan surface: %s\n", SDL_GetError());
        exit(1);
    }

    vkb::PhysicalDeviceSelector phys_selector{inst_ret.value()};
    phys_selector.set_surface(*surface)
                 .set_minimum_version(1, 3)
                 .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
    auto phys_ret = phys_selector.select();
    if (!phys_ret) {
        fprintf(stderr, "Failed to select physical device: %s\n", phys_ret.error().message().c_str());
        exit(1);
    }
    *physicalDevice = phys_ret.value();

    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        fprintf(stderr, "Failed to create Vulkan device: %s\n", dev_ret.error().message().c_str());
        exit(1);
    }
    *device = dev_ret.value();

    auto queue_ret = dev_ret.value().get_queue(vkb::QueueType::graphics);
    if (!queue_ret) {
        fprintf(stderr, "Failed to get graphics queue: %s\n", queue_ret.error().message().c_str());
        exit(1);
    }
    *graphicsQueue = queue_ret.value();
    *graphicsQueueFamilyIndex = dev_ret.value().get_queue_index(vkb::QueueType::graphics).value();

    vkb::SwapchainBuilder swapchain_builder{dev_ret.value()};
    swapchain_builder.set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                     .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                     .set_desired_extent(800, 600);
    auto swap_ret = swapchain_builder.build();
    if (!swap_ret) {
        fprintf(stderr, "Failed to create swapchain: %s\n", swap_ret.error().message().c_str());
        exit(1);
    }
    *swapchain = swap_ret.value().swapchain;
    *imageCount = swap_ret.value().image_count;
    *swapchainImages = swap_ret.value().get_images().value().data();

    auto image_views = swap_ret.value().get_image_views().value();
    *swapchainImageViews = (VkImageView*)malloc(*imageCount * sizeof(VkImageView));
    if (!*swapchainImageViews) {
        fprintf(stderr, "Failed to allocate memory for swapchain image views\n");
        exit(1);
    }
    for (uint32_t i = 0; i < *imageCount; i++) {
        (*swapchainImageViews)[i] = image_views[i];
    }

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