#include "vsdl_init.h"
#include <stdlib.h>

static int create_instance(void) {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    const char* extensions[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = 0
    };

    VkResult result = vkCreateInstance(&createInfo, NULL, &g_context.instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("vkCreateInstance failed: %d", result);
        return 1;
    }
    LOG("Vulkan instance created");
    return 0;
}

static int pick_physical_device(void) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(g_context.instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        LOG_ERROR("No physical devices found");
        return 1;
    }
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    if (!devices) {
        LOG_ERROR("Failed to allocate memory for devices");
        return 1;
    }
    vkEnumeratePhysicalDevices(g_context.instance, &deviceCount, devices);
    g_context.physicalDevice = devices[0]; // Pick first device
    free(devices);
    LOG("Physical device selected");
    return 0;
}

static int create_logical_device(void) {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_context.physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    if (!queueFamilies) {
        LOG_ERROR("Failed to allocate memory for queue families");
        return 1;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(g_context.physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t graphicsFamily = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }
    free(queueFamilies);
    if (graphicsFamily == -1) {
        LOG_ERROR("No graphics queue family found");
        return 1;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };
    const char* deviceExtensions[] = {"VK_KHR_swapchain"};  // Enable swapchain extension
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExtensions
    };
    if (vkCreateDevice(g_context.physicalDevice, &createInfo, NULL, &g_context.device) != VK_SUCCESS) {
        LOG_ERROR("vkCreateDevice failed");
        return 1;
    }
    vkGetDeviceQueue(g_context.device, graphicsFamily, 0, &g_context.graphicsQueue);
    LOG("Logical device created");
    return 0;
}

static int create_swapchain(void) {
    VkSurfaceCapabilitiesKHR capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_context.physicalDevice, g_context.surface, &capabilities) != VK_SUCCESS) {
        LOG_ERROR("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
        return 1;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_context.surface,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_SRGB,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = capabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE
    };
    if (vkCreateSwapchainKHR(g_context.device, &createInfo, NULL, &g_context.swapchain) != VK_SUCCESS) {
        LOG_ERROR("vkCreateSwapchainKHR failed");
        return 1;
    }

    vkGetSwapchainImagesKHR(g_context.device, g_context.swapchain, &g_context.swapchainImageCount, NULL);
    g_context.swapchainImages = malloc(g_context.swapchainImageCount * sizeof(VkImage));
    if (!g_context.swapchainImages) {
        LOG_ERROR("Failed to allocate memory for swapchain images");
        return 1;
    }
    vkGetSwapchainImagesKHR(g_context.device, g_context.swapchain, &g_context.swapchainImageCount, g_context.swapchainImages);

    g_context.swapchainImageViews = malloc(g_context.swapchainImageCount * sizeof(VkImageView));
    if (!g_context.swapchainImageViews) {
        LOG_ERROR("Failed to allocate memory for swapchain image views");
        return 1;
    }
    for (uint32_t i = 0; i < g_context.swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = g_context.swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_SRGB,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.levelCount = 1,
            .subresourceRange.layerCount = 1
        };
        if (vkCreateImageView(g_context.device, &viewInfo, NULL, &g_context.swapchainImageViews[i]) != VK_SUCCESS) {
            LOG_ERROR("vkCreateImageView failed");
            return 1;
        }
    }
    LOG("Swapchain created");
    return 0;
}

int vsdl_init(void) {
    LOG("Initializing VSDL");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    g_context.window = SDL_CreateWindow("Vulkan Triangle", 800, 600, SDL_WINDOW_VULKAN);
    if (!g_context.window) {
        LOG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    if (create_instance()) {
        LOG_ERROR("create_instance failed");
        SDL_DestroyWindow(g_context.window);
        SDL_Quit();
        return 1;
    }
    if (!SDL_Vulkan_CreateSurface(g_context.window, g_context.instance, NULL, &g_context.surface)) {
        LOG_ERROR("SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
        SDL_DestroyWindow(g_context.window);
        SDL_Quit();
        return 1;
    }
    if (pick_physical_device()) {
        LOG_ERROR("pick_physical_device failed");
        SDL_DestroyWindow(g_context.window);
        SDL_Quit();
        return 1;
    }
    if (create_logical_device()) {
        LOG_ERROR("create_logical_device failed");
        SDL_DestroyWindow(g_context.window);
        SDL_Quit();
        return 1;
    }
    if (create_swapchain()) {
        LOG_ERROR("create_swapchain failed");
        SDL_DestroyWindow(g_context.window);
        SDL_Quit();
        return 1;
    }
    LOG("VSDL initialized successfully");
    return 0;
}