#define VK_NO_PROTOTYPES
#define VOLK_IMPLEMENTATION

#include <vulkan/vulkan.h>
#include <volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vk_mem_alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_INFO(...) printf("[INFO] " __VA_ARGS__)
#define LOG_ERROR(...) printf("[ERROR] " __VA_ARGS__)

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VmaAllocator allocator;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence fence;
    uint32_t queueFamilyIndex;
    VkImage* swapchainImages;
    uint32_t swapchainImageCount;
    VkImageView* imageViews;
    VkFramebuffer* framebuffers;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    VkDebugUtilsMessengerEXT debugMessenger;
} VulkanContext;

typedef struct {
    SDL_Window *window;
    VulkanContext context;
    bool shouldClose;
} App;

static VkResult checkVkResult(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        LOG_ERROR("%s failed with result: %d\n", operation, result);
    }
    return result;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOG_ERROR("Validation layer: %s\n", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

static VkShaderModule createShaderModule(VkDevice device, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        LOG_ERROR("Failed to open shader file: %s\n", filename);
        return VK_NULL_HANDLE;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* code = malloc(size);
    fread(code, 1, size, file);
    fclose(file);

    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (uint32_t*)code,
    };

    VkShaderModule module;
    if (checkVkResult(vkCreateShaderModule(device, &createInfo, NULL, &module), "vkCreateShaderModule") == VK_SUCCESS) {
        LOG_INFO("Loaded shader: %s\n", filename);
    }
    free(code);
    return module;
}

static void setupDebugMessenger(VulkanContext* ctx) {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = NULL
    };

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        checkVkResult(func(ctx->instance, &createInfo, NULL, &ctx->debugMessenger), "vkCreateDebugUtilsMessengerEXT");
    } else {
        LOG_ERROR("Failed to set up debug messenger: extension not present\n");
    }
}

void createVulkanContext(VulkanContext *ctx, SDL_Window *window, Uint32 apiVersion) {
    if (!SDL_Vulkan_LoadLibrary(NULL)) {
        LOG_ERROR("Failed to load Vulkan library: %s\n", SDL_GetError());
        return;
    }
    
    volkInitializeCustom((PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr());

    Uint32 extensionsCount;
    char const *const *requiredExtensions = SDL_Vulkan_GetInstanceExtensions(&extensionsCount);
    
    // Validation layers and extensions
    const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
    const char* additionalExtensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    uint32_t totalExtensionCount = extensionsCount + 1; // SDL extensions + debug utils
    const char** allExtensions = malloc(totalExtensionCount * sizeof(const char*));

    // Copy SDL extensions
    for (uint32_t i = 0; i < extensionsCount; i++) {
        allExtensions[i] = requiredExtensions[i];
    }
    // Add debug extension
    allExtensions[extensionsCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = totalExtensionCount,
        .ppEnabledExtensionNames = allExtensions,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .apiVersion = apiVersion,
            .pApplicationName = "SDL3 Vulkan Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        },
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = validationLayers,
    };

    if (checkVkResult(vkCreateInstance(&instanceInfo, NULL, &ctx->instance), "vkCreateInstance") != VK_SUCCESS) {
        free(allExtensions);
        return;
    }
    free(allExtensions);
    volkLoadInstanceOnly(ctx->instance);

    setupDebugMessenger(ctx);

    uint32_t physicalDeviceCount = 1;
    if (checkVkResult(vkEnumeratePhysicalDevices(ctx->instance, &physicalDeviceCount, &ctx->physicalDevice), 
                     "vkEnumeratePhysicalDevices") != VK_SUCCESS || physicalDeviceCount == 0) {
        LOG_ERROR("No suitable physical device found\n");
        return;
    }

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);
    
    ctx->queueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            ctx->queueFamilyIndex = i;
            break;
        }
    }
    free(queueFamilies);
    if (ctx->queueFamilyIndex == UINT32_MAX) {
        LOG_ERROR("No graphics queue family found\n");
        return;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = ctx->queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExtensions,
    };

    if (checkVkResult(vkCreateDevice(ctx->physicalDevice, &deviceInfo, NULL, &ctx->device), 
                     "vkCreateDevice") != VK_SUCCESS) {
        return;
    }
    volkLoadDevice(ctx->device);

    vkGetDeviceQueue(ctx->device, ctx->queueFamilyIndex, 0, &ctx->queue);

    VmaVulkanFunctions vmaVulkanFuncs = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory = vkAllocateMemory,
        .vkFreeMemory = vkFreeMemory,
        .vkMapMemory = vkMapMemory,
        .vkUnmapMemory = vkUnmapMemory,
        .vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory = vkBindBufferMemory,
        .vkBindImageMemory = vkBindImageMemory,
        .vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
        .vkCreateBuffer = vkCreateBuffer,
        .vkDestroyBuffer = vkDestroyBuffer,
        .vkCreateImage = vkCreateImage,
        .vkDestroyImage = vkDestroyImage,
        .vkCmdCopyBuffer = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2,
        .vkBindBufferMemory2KHR = vkBindBufferMemory2,
        .vkBindImageMemory2KHR = vkBindImageMemory2,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2,
        .vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements,
    };

    VmaAllocatorCreateInfo allocatorInfo = {
        .physicalDevice = ctx->physicalDevice,
        .device = ctx->device,
        .instance = ctx->instance,
        .vulkanApiVersion = apiVersion,
        .pVulkanFunctions = &vmaVulkanFuncs,
    };

    if (vmaCreateAllocator(&allocatorInfo, &ctx->allocator) != VK_SUCCESS) {
        LOG_ERROR("Failed to create VMA allocator\n");
        return;
    }
    LOG_INFO("Vulkan context and VMA allocator created successfully\n");

    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = ctx->queueFamilyIndex,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    checkVkResult(vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ctx->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    checkVkResult(vkAllocateCommandBuffers(ctx->device, &cmdBufInfo, &ctx->commandBuffer), "vkAllocateCommandBuffers");

    VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo = { 
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT 
    };
    checkVkResult(vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphore), "vkCreateSemaphore");
    checkVkResult(vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphore), "vkCreateSemaphore");
    checkVkResult(vkCreateFence(ctx->device, &fenceInfo, NULL, &ctx->fence), "vkCreateFence");
}

void createSwapchainAndPipeline(VulkanContext* ctx, SDL_Window* window) {
    if (!SDL_Vulkan_CreateSurface(window, ctx->instance, NULL, &ctx->surface)) {
        LOG_ERROR("Failed to create Vulkan surface: %s\n", SDL_GetError());
        return;
    }

    VkAttachmentDescription attachment = {
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachment = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };
    checkVkResult(vkCreateRenderPass(ctx->device, &renderPassInfo, NULL, &ctx->renderPass), "vkCreateRenderPass");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = ctx->surface,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = {(uint32_t)width, (uint32_t)height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
    };
    checkVkResult(vkCreateSwapchainKHR(ctx->device, &swapchainInfo, NULL, &ctx->swapchain), "vkCreateSwapchainKHR");

    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, NULL);
    ctx->swapchainImages = malloc(ctx->swapchainImageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->swapchainImageCount, ctx->swapchainImages);

    ctx->imageViews = malloc(ctx->swapchainImageCount * sizeof(VkImageView));
    ctx->framebuffers = malloc(ctx->swapchainImageCount * sizeof(VkFramebuffer));

    for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = ctx->swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                          VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
        checkVkResult(vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->imageViews[i]), "vkCreateImageView");

        VkFramebufferCreateInfo fbInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = ctx->renderPass,
            .attachmentCount = 1,
            .pAttachments = &ctx->imageViews[i],
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .layers = 1,
        };
        checkVkResult(vkCreateFramebuffer(ctx->device, &fbInfo, NULL, &ctx->framebuffers[i]), "vkCreateFramebuffer");
    }

    ctx->vertShaderModule = createShaderModule(ctx->device, "shaders/triangle.vert.spv");
    ctx->fragShaderModule = createShaderModule(ctx->device, "shaders/triangle.frag.spv");

    if (!ctx->vertShaderModule || !ctx->fragShaderModule) {
        LOG_ERROR("Failed to create shader modules\n");
        return;
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = ctx->vertShaderModule,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = ctx->fragShaderModule,
            .pName = "main",
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)width,
        .height = (float)height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {(uint32_t)width, (uint32_t)height},
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0,
    };
    checkVkResult(vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->pipelineLayout), "vkCreatePipelineLayout");

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .layout = ctx->pipelineLayout,
        .renderPass = ctx->renderPass,
        .subpass = 0,
    };
    checkVkResult(vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->pipeline), "vkCreateGraphicsPipelines");
}

void render(App* app) {
    VulkanContext* ctx = &app->context;
    
    vkWaitForFences(ctx->device, 1, &ctx->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(ctx->device, 1, &ctx->fence);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, 
                                           ctx->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS) {
        LOG_ERROR("vkAcquireNextImageKHR failed: %d\n", result);
        return;
    }

    vkResetCommandBuffer(ctx->commandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(ctx->commandBuffer, &beginInfo);

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    int width, height;
    SDL_GetWindowSize(app->window, &width, &height);
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = ctx->renderPass,
        .framebuffer = ctx->framebuffers[imageIndex],
        .renderArea.extent = {(uint32_t)width, (uint32_t)height},
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };
    vkCmdBeginRenderPass(ctx->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipeline);
    vkCmdDraw(ctx->commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(ctx->commandBuffer);
    vkEndCommandBuffer(ctx->commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ctx->imageAvailableSemaphore,
        .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &ctx->commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &ctx->renderFinishedSemaphore,
    };
    vkQueueSubmit(ctx->queue, 1, &submitInfo, ctx->fence);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ctx->renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &ctx->swapchain,
        .pImageIndices = &imageIndex,
    };
    vkQueuePresentKHR(ctx->queue, &presentInfo);
}

void cleanup(App* app) {
    VulkanContext* ctx = &app->context;
    
    vkDeviceWaitIdle(ctx->device);

    if (ctx->pipeline) vkDestroyPipeline(ctx->device, ctx->pipeline, NULL);
    if (ctx->pipelineLayout) vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, NULL);
    if (ctx->vertShaderModule) vkDestroyShaderModule(ctx->device, ctx->vertShaderModule, NULL);
    if (ctx->fragShaderModule) vkDestroyShaderModule(ctx->device, ctx->fragShaderModule, NULL);
    if (ctx->framebuffers) {
        for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
            vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
        }
        free(ctx->framebuffers);
    }
    if (ctx->imageViews) {
        for (uint32_t i = 0; i < ctx->swapchainImageCount; i++) {
            vkDestroyImageView(ctx->device, ctx->imageViews[i], NULL);
        }
        free(ctx->imageViews);
    }
    if (ctx->swapchainImages) free(ctx->swapchainImages);
    if (ctx->renderPass) vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
    if (ctx->swapchain) vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
    if (ctx->surface) vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    if (ctx->imageAvailableSemaphore) vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphore, NULL);
    if (ctx->renderFinishedSemaphore) vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphore, NULL);
    if (ctx->fence) vkDestroyFence(ctx->device, ctx->fence, NULL);
    if (ctx->commandPool) vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
    if (ctx->allocator) vmaDestroyAllocator(ctx->allocator);
    if (ctx->device) vkDestroyDevice(ctx->device, NULL);
    if (ctx->debugMessenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(ctx->instance, ctx->debugMessenger, NULL);
    }
    if (ctx->instance) vkDestroyInstance(ctx->instance, NULL);
    
    if (app->window) SDL_DestroyWindow(app->window);
    SDL_Quit();
}

int main(int argc, char **argv) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    App app = {0};
    app.window = SDL_CreateWindow("SDL3 Vulkan Triangle", 800, 600,
                                 SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!app.window) {
        LOG_ERROR("Failed to create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_ShowWindow(app.window);

    createVulkanContext(&app.context, app.window, VK_API_VERSION_1_2);
    if (!app.context.device) {
        cleanup(&app);
        return 1;
    }

    createSwapchainAndPipeline(&app.context, app.window);
    if (!app.context.swapchain) {
        cleanup(&app);
        return 1;
    }

    while (!app.shouldClose) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                app.shouldClose = true;
            }
        }
        render(&app);
    }

    cleanup(&app);
    return 0;
}