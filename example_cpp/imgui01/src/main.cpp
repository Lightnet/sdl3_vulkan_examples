#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include <stdio.h>
#include <vector>
#include <stdexcept>

// Vulkan function loader for ImGui
static PFN_vkVoidFunction ImGui_ImplVulkan_Loader(const char* function_name, void* user_data) {
    VkInstance instance = static_cast<VkInstance>(user_data);
    PFN_vkVoidFunction func = vkGetInstanceProcAddr(instance, function_name);
    if (!func) {
        printf("Error: Failed to load Vulkan function: %s\n", function_name);
    }
    return func;
}

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;
    uint32_t graphicsFamily = UINT32_MAX;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
};

void cleanupVulkan(VulkanContext& ctx, SDL_Window* window) {
    printf("Cleaning up Vulkan resources...\n");
    if (ctx.device) vkDeviceWaitIdle(ctx.device);

    if (ctx.renderFinishedSemaphore) vkDestroySemaphore(ctx.device, ctx.renderFinishedSemaphore, nullptr);
    if (ctx.imageAvailableSemaphore) vkDestroySemaphore(ctx.device, ctx.imageAvailableSemaphore, nullptr);
    if (ctx.inFlightFence) vkDestroyFence(ctx.device, ctx.inFlightFence, nullptr);
    if (ctx.commandPool) vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
    
    for (auto framebuffer : ctx.framebuffers) {
        if (framebuffer) vkDestroyFramebuffer(ctx.device, framebuffer, nullptr);
    }
    if (ctx.renderPass) vkDestroyRenderPass(ctx.device, ctx.renderPass, nullptr);
    
    for (auto imageView : ctx.swapchainImageViews) {
        if (imageView) vkDestroyImageView(ctx.device, imageView, nullptr);
    }
    if (ctx.swapchain) vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);
    if (ctx.device) vkDestroyDevice(ctx.device, nullptr);
    if (ctx.surface && ctx.instance) vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
    if (ctx.instance) vkDestroyInstance(ctx.instance, nullptr);
    
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    printf("Vulkan cleanup completed.\n");
}

void initVulkan(SDL_Window* window, VulkanContext& ctx) {
    printf("Initializing Vulkan...\n");

    // Create Instance
    printf("Creating Vulkan instance...\n");
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "ImGui SDL3 Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t extensionCount = 0;
    const char *const *extensionNamesConst = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensionNamesConst || extensionCount == 0) {
        printf("Error: Failed to get Vulkan instance extensions\n");
        throw std::runtime_error("Failed to get Vulkan instance extensions");
    }
    printf("Found %u Vulkan instance extensions\n", extensionCount);
    std::vector<const char*> extensionNames(extensionNamesConst, extensionNamesConst + extensionCount);

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    if (vkCreateInstance(&createInfo, nullptr, &ctx.instance) != VK_SUCCESS) {
        printf("Error: Failed to create Vulkan instance\n");
        throw std::runtime_error("Failed to create Vulkan instance");
    }
    printf("Vulkan instance created successfully\n");

    // Create Surface
    printf("Creating Vulkan surface...\n");
    if (!SDL_Vulkan_CreateSurface(window, ctx.instance, nullptr, &ctx.surface)) {
        printf("Error: Failed to create Vulkan surface - %s\n", SDL_GetError());
        throw std::runtime_error("Failed to create Vulkan surface");
    }
    printf("Vulkan surface created successfully\n");

    // Pick Physical Device
    printf("Selecting physical device...\n");
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        printf("Error: No Vulkan-capable devices found\n");
        throw std::runtime_error("No Vulkan-capable devices found");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(ctx.instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, ctx.surface, &presentSupport);
                if (presentSupport) {
                    ctx.physicalDevice = device;
                    ctx.graphicsFamily = i;
                    break;
                }
            }
        }
        if (ctx.physicalDevice != VK_NULL_HANDLE) break;
    }

    if (ctx.physicalDevice == VK_NULL_HANDLE) {
        printf("Error: Failed to find suitable GPU\n");
        throw std::runtime_error("Failed to find suitable GPU");
    }
    printf("Physical device selected successfully\n");

    // Create Logical Device with VK_KHR_swapchain extension
    printf("Creating logical device...\n");
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = ctx.graphicsFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    const char* deviceExtensions[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(ctx.physicalDevice, &deviceCreateInfo, nullptr, &ctx.device) != VK_SUCCESS) {
        printf("Error: Failed to create logical device\n");
        throw std::runtime_error("Failed to create logical device");
    }
    printf("Logical device created successfully\n");

    vkGetDeviceQueue(ctx.device, ctx.graphicsFamily, 0, &ctx.graphicsQueue);
    printf("Graphics queue retrieved\n");

    // Create Swapchain
    printf("Creating swapchain...\n");
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice, ctx.surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }

    ctx.swapchainImageFormat = surfaceFormat.format;
    ctx.swapchainExtent = capabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = ctx.surface;
    swapchainCreateInfo.minImageCount = capabilities.minImageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = capabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(ctx.device, &swapchainCreateInfo, nullptr, &ctx.swapchain) != VK_SUCCESS) {
        printf("Error: Failed to create swapchain\n");
        throw std::runtime_error("Failed to create swapchain");
    }
    printf("Swapchain created successfully\n");

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageCount, nullptr);
    ctx.swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &imageCount, ctx.swapchainImages.data());
    printf("Retrieved %u swapchain images\n", imageCount);

    // Create Image Views
    printf("Creating swapchain image views...\n");
    ctx.swapchainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = ctx.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = surfaceFormat.format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &ctx.swapchainImageViews[i]) != VK_SUCCESS) {
            printf("Error: Failed to create image view %zu\n", i);
            throw std::runtime_error("Failed to create image views");
        }
    }
    printf("Swapchain image views created successfully\n");

    // Create Render Pass
    printf("Creating render pass...\n");
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.renderPass) != VK_SUCCESS) {
        printf("Error: Failed to create render pass\n");
        throw std::runtime_error("Failed to create render pass");
    }
    printf("Render pass created successfully\n");

    // Create Framebuffers
    printf("Creating framebuffers...\n");
    ctx.framebuffers.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = ctx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &ctx.swapchainImageViews[i];
        framebufferInfo.width = ctx.swapchainExtent.width;
        framebufferInfo.height = ctx.swapchainExtent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &ctx.framebuffers[i]) != VK_SUCCESS) {
            printf("Error: Failed to create framebuffer %zu\n", i);
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
    printf("Framebuffers created successfully\n");

    // Create Command Pool
    printf("Creating command pool...\n");
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = ctx.graphicsFamily;
    if (vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS) {
        printf("Error: Failed to create command pool\n");
        throw std::runtime_error("Failed to create command pool");
    }
    printf("Command pool created successfully\n");

    // Create Command Buffer
    printf("Allocating command buffer...\n");
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(ctx.device, &allocInfo, &ctx.commandBuffer) != VK_SUCCESS) {
        printf("Error: Failed to allocate command buffer\n");
        throw std::runtime_error("Failed to allocate command buffer");
    }
    printf("Command buffer allocated successfully\n");

    // Create Sync Objects
    printf("Creating synchronization objects...\n");
    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.imageAvailableSemaphore) != VK_SUCCESS) {
        printf("Error: Failed to create image available semaphore\n");
        throw std::runtime_error("Failed to create synchronization objects");
    }
    if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.renderFinishedSemaphore) != VK_SUCCESS) {
        printf("Error: Failed to create render finished semaphore\n");
        throw std::runtime_error("Failed to create synchronization objects");
    }
    if (vkCreateFence(ctx.device, &fenceInfo, nullptr, &ctx.inFlightFence) != VK_SUCCESS) {
        printf("Error: Failed to create in-flight fence\n");
        throw std::runtime_error("Failed to create synchronization objects");
    }
    printf("Synchronization objects created successfully\n");
}

int main(int, char**) {
    printf("Starting ImGuiSDL3Vulkan...\n");

    // SDL Initialization
    printf("Initializing SDL...\n");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error: SDL initialization failed - %s\n", SDL_GetError());
        return -1;
    }
    printf("SDL initialized successfully\n");

    // Create Window
    printf("Creating SDL window...\n");
    SDL_Window* window = SDL_CreateWindow("ImGui SDL3 Vulkan",
                                        1280, 720,
                                        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Error: Failed to create window - %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    printf("SDL window created successfully\n");

    // Vulkan Initialization
    VulkanContext vkCtx;
    try {
        initVulkan(window, vkCtx);
    } catch (const std::runtime_error& e) {
        printf("Vulkan Error: %s\n", e.what());
        cleanupVulkan(vkCtx, window);
        return -1;
    }

    // Setup ImGui
    printf("Initializing ImGui...\n");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    printf("ImGui context created\n");

    printf("Initializing ImGui SDL3 backend...\n");
    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        printf("Error: Failed to initialize ImGui SDL3 backend\n");
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    printf("ImGui SDL3 backend initialized\n");

    // Load Vulkan functions for ImGui
    printf("Loading Vulkan functions for ImGui...\n");
    if (!ImGui_ImplVulkan_LoadFunctions(ImGui_ImplVulkan_Loader, vkCtx.instance)) {
        printf("Error: Failed to load Vulkan functions for ImGui\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    printf("Vulkan functions loaded for ImGui\n");

    // Setup ImGui Vulkan Backend
    printf("Setting up ImGui Vulkan backend...\n");
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkCtx.instance;
    init_info.PhysicalDevice = vkCtx.physicalDevice;
    init_info.Device = vkCtx.device;
    init_info.QueueFamily = vkCtx.graphicsFamily;
    init_info.Queue = vkCtx.graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.RenderPass = vkCtx.renderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(vkCtx.swapchainImages.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;

    // Create Descriptor Pool
    printf("Creating descriptor pool for ImGui...\n");
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(vkCtx.device, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
        printf("Error: Failed to create descriptor pool\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    init_info.DescriptorPool = descriptorPool;
    printf("Descriptor pool created successfully\n");

    // Initialize ImGui Vulkan Backend
    printf("Initializing ImGui Vulkan backend...\n");
    if (!ImGui_ImplVulkan_Init(&init_info)) {
        printf("Error: Failed to initialize ImGui Vulkan backend\n");
        vkDestroyDescriptorPool(vkCtx.device, descriptorPool, nullptr);
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    printf("ImGui Vulkan backend initialized\n");

    // Upload Fonts
    printf("Uploading ImGui fonts...\n");
    VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(vkCtx.commandBuffer, &begin_info) != VK_SUCCESS) {
        printf("Error: Failed to begin command buffer for font upload\n");
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(vkCtx.device, descriptorPool, nullptr);
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    ImGui_ImplVulkan_CreateFontsTexture();
    if (vkEndCommandBuffer(vkCtx.commandBuffer) != VK_SUCCESS) {
        printf("Error: Failed to end command buffer for font upload\n");
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(vkCtx.device, descriptorPool, nullptr);
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vkCtx.commandBuffer;
    if (vkQueueSubmit(vkCtx.graphicsQueue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        printf("Error: Failed to submit command buffer for font upload\n");
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(vkCtx.device, descriptorPool, nullptr);
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    if (vkQueueWaitIdle(vkCtx.graphicsQueue) != VK_SUCCESS) {
        printf("Error: Failed to wait for queue idle after font upload\n");
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(vkCtx.device, descriptorPool, nullptr);
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        cleanupVulkan(vkCtx, window);
        return -1;
    }
    printf("ImGui fonts uploaded successfully\n");

    // Main loop
    printf("Entering main loop...\n");
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                printf("Received quit event\n");
                done = true;
            }
        }

        if (vkWaitForFences(vkCtx.device, 1, &vkCtx.inFlightFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
            printf("Error: Failed to wait for in-flight fence\n");
            break;
        }
        if (vkResetFences(vkCtx.device, 1, &vkCtx.inFlightFence) != VK_SUCCESS) {
            printf("Error: Failed to reset in-flight fence\n");
            break;
        }

        uint32_t imageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(vkCtx.device, vkCtx.swapchain, UINT64_MAX, 
                                                      vkCtx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (acquireResult != VK_SUCCESS) {
            printf("Error: Failed to acquire next image - VkResult: %d\n", acquireResult);
            break;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        static float f = 0.0f;
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        if (ImGui::Button("Click me")) {
          printf("Button clicked!\n");
        }
        static char buf[32] = "";
        ImGui::InputText("Input", buf, 32);
        ImGui::End();

        ImGui::Render();

        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (vkBeginCommandBuffer(vkCtx.commandBuffer, &beginInfo) != VK_SUCCESS) {
            printf("Error: Failed to begin command buffer for rendering\n");
            break;
        }

        VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = vkCtx.renderPass;
        renderPassInfo.framebuffer = vkCtx.framebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vkCtx.swapchainExtent;
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(vkCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkCtx.commandBuffer);
        vkCmdEndRenderPass(vkCtx.commandBuffer);
        if (vkEndCommandBuffer(vkCtx.commandBuffer) != VK_SUCCESS) {
            printf("Error: Failed to end command buffer for rendering\n");
            break;
        }

        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &vkCtx.imageAvailableSemaphore;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCtx.commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &vkCtx.renderFinishedSemaphore;
        if (vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, vkCtx.inFlightFence) != VK_SUCCESS) {
            printf("Error: Failed to submit render command buffer\n");
            break;
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &vkCtx.renderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkCtx.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        VkResult presentResult = vkQueuePresentKHR(vkCtx.graphicsQueue, &presentInfo);
        if (presentResult != VK_SUCCESS) {
            printf("Error: Failed to present - VkResult: %d\n", presentResult);
            break;
        }
    }

    // Cleanup
    printf("Shutting down...\n");
    vkDeviceWaitIdle(vkCtx.device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(vkCtx.device, descriptorPool, nullptr);
    cleanupVulkan(vkCtx, window);

    printf("Program exited\n");
    return 0;
}