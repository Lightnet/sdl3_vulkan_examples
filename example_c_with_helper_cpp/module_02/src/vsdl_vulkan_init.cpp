#include "vsdl_vulkan_init.h"
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

    // Use vkb::Device for SwapchainBuilder
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

extern "C" void recreate_swapchain(VulkanContext* vkCtx, SDL_Window* window, uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(vkCtx->device);

    // Destroy old resources
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        vkDestroyFramebuffer(vkCtx->device, vkCtx->swapchainFramebuffers[i], NULL);
    }
    free(vkCtx->swapchainFramebuffers);
    vkDestroyImageView(vkCtx->device, vkCtx->depthImageView, NULL);
    vmaDestroyImage(allocator, vkCtx->depthImage, vkCtx->depthAllocation);
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        vkDestroyImageView(vkCtx->device, vkCtx->swapchainImageViews[i], NULL);
    }
    free(vkCtx->swapchainImageViews);
    vkDestroySwapchainKHR(vkCtx->device, vkCtx->swapchain, NULL);

    // Reconstruct vkb::Device from VulkanContext
    vkb::Device vkb_device;
    vkb_device.device = vkCtx->device;
    vkb_device.physical_device.physical_device = vkCtx->physicalDevice;
    vkb_device.surface = vkCtx->surface;

    // Recreate swapchain using vkb::Device
    vkb::SwapchainBuilder swapchain_builder{vkb_device};
    swapchain_builder.set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                     .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                     .set_desired_extent(width, height);
    auto swap_ret = swapchain_builder.build();
    if (!swap_ret) {
        fprintf(stderr, "Failed to recreate swapchain: %s\n", swap_ret.error().message().c_str());
        exit(1);
    }
    vkCtx->swapchain = swap_ret.value().swapchain;
    vkCtx->imageCount = swap_ret.value().image_count;
    vkCtx->swapchainImages = swap_ret.value().get_images().value().data();

    auto image_views = swap_ret.value().get_image_views().value();
    vkCtx->swapchainImageViews = (VkImageView*)malloc(vkCtx->imageCount * sizeof(VkImageView));
    if (!vkCtx->swapchainImageViews) {
        fprintf(stderr, "Failed to allocate memory for swapchain image views\n");
        exit(1);
    }
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        vkCtx->swapchainImageViews[i] = image_views[i];
    }

    // Recreate depth image
    VkImageCreateInfo depthImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
    depthImageInfo.extent.width = width;
    depthImageInfo.extent.height = height;
    depthImageInfo.extent.depth = 1;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo depthAllocInfo = {};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    if (vmaCreateImage(allocator, &depthImageInfo, &depthAllocInfo, &vkCtx->depthImage, &vkCtx->depthAllocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to recreate depth image\n");
        exit(1);
    }

    VkImageViewCreateInfo depthViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    depthViewInfo.image = vkCtx->depthImage;
    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthViewInfo.subresourceRange.baseMipLevel = 0;
    depthViewInfo.subresourceRange.levelCount = 1;
    depthViewInfo.subresourceRange.baseArrayLayer = 0;
    depthViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vkCtx->device, &depthViewInfo, NULL, &vkCtx->depthImageView) != VK_SUCCESS) {
        fprintf(stderr, "Failed to recreate depth image view\n");
        exit(1);
    }

    VkCommandBuffer depthCmd;
    VkCommandBufferAllocateInfo depthAllocInfoCmd = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    depthAllocInfoCmd.commandPool = vkCtx->commandPool;
    depthAllocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    depthAllocInfoCmd.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkCtx->device, &depthAllocInfoCmd, &depthCmd);

    VkCommandBufferBeginInfo depthBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    depthBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(depthCmd, &depthBeginInfo);

    VkImageMemoryBarrier depthBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.image = vkCtx->depthImage;
    depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthBarrier.subresourceRange.baseMipLevel = 0;
    depthBarrier.subresourceRange.levelCount = 1;
    depthBarrier.subresourceRange.baseArrayLayer = 0;
    depthBarrier.subresourceRange.layerCount = 1;
    depthBarrier.srcAccessMask = 0;
    depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(depthCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, NULL, 0, NULL, 1, &depthBarrier);
    vkEndCommandBuffer(depthCmd);

    VkSubmitInfo depthSubmit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    depthSubmit.commandBufferCount = 1;
    depthSubmit.pCommandBuffers = &depthCmd;
    vkQueueSubmit(vkCtx->graphicsQueue, 1, &depthSubmit, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkCtx->graphicsQueue);
    vkFreeCommandBuffers(vkCtx->device, vkCtx->commandPool, 1, &depthCmd);

    vkCtx->swapchainFramebuffers = (VkFramebuffer*)malloc(vkCtx->imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        VkImageView attachments[] = {vkCtx->swapchainImageViews[i], vkCtx->depthImageView};
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx->renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx->device, &framebufferInfo, NULL, &vkCtx->swapchainFramebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to recreate framebuffer %u\n", i);
            exit(1);
        }
    }
}