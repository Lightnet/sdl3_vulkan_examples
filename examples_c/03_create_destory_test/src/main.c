#include <SDL3/SDL.h>           // SDL3 for windowing and input handling
#include <SDL3/SDL_vulkan.h>    // SDL3 Vulkan extension for surface creation
#include <vulkan/vulkan.h>      // Core Vulkan API for graphics
#include <cglm/cglm.h>          // Pure C math library for vectors and matrices
#include <stdio.h>              // Standard I/O for logging and debugging
#include <stdlib.h>             // Standard library for memory allocation (malloc, exit)
#include <string.h>             // String operations (memcpy for buffer data)
#include <stdbool.h>            // Boolean type for flags (true/false)

// Window dimensions
#define WIDTH 800
#define HEIGHT 600

// Camera structure to manage position and orientation
typedef struct {
    vec3 pos;      // Position in 3D space (x, y, z)
    vec3 front;    // Forward direction vector (where camera looks)
    vec3 up;       // Up direction vector (defines camera roll)
    float yaw;     // Horizontal rotation angle (degrees)
    float pitch;   // Vertical rotation angle (degrees)
} Camera;

// Structure to manage a renderable object
typedef struct {
    VkBuffer buffer;        // Vertex buffer handle
    VkDeviceMemory memory;  // Memory handle for the buffer
    uint32_t vertexCount;   // Number of vertices
    bool exists;            // Flag to track existence
} RenderObject;

// Vulkan context to hold all Vulkan objects
struct VulkanContext {
    VkInstance instance;                    // Vulkan instance handle
    VkPhysicalDevice physicalDevice;        // Selected GPU
    VkDevice device;                        // Logical device for commands
    VkQueue graphicsQueue;                  // Queue for graphics operations
    VkSurfaceKHR surface;                   // Window surface for rendering
    VkSwapchainKHR swapchain;               // Swapchain for double buffering
    VkRenderPass renderPass;                // Render pass for pipeline
    VkPipelineLayout pipelineLayout;        // Pipeline layout with descriptor sets
    VkPipeline graphicsPipeline;            // Graphics pipeline for rendering
    VkCommandPool commandPool;              // Pool for allocating command buffers
    VkCommandBuffer commandBuffer;          // Buffer for recording draw commands
    VkBuffer uniformBuffer;                 // Buffer for uniform data (matrices)
    VkDeviceMemory uniformMemory;           // Memory for uniform buffer
    VkDescriptorSetLayout descriptorSetLayout; // Layout for uniform bindings
    VkDescriptorPool descriptorPool;        // Pool for descriptor sets
    VkDescriptorSet descriptorSet;          // Descriptor set for UBO
    VkSemaphore imageAvailableSemaphore;    // Signals when swapchain image is ready
    VkSemaphore renderFinishedSemaphore;    // Signals when rendering is done
    VkFence inFlightFence;                  // Syncs CPU with GPU frame completion
    uint32_t imageCount;                    // Number of swapchain images
    VkImage* swapchainImages;               // Array of swapchain images
    VkImageView* swapchainImageViews;       // Views for swapchain images
    VkFramebuffer* swapchainFramebuffers;   // Framebuffers for rendering
    RenderObject triangle;                  // Triangle object
    RenderObject cube;                      // Cube object
} vkCtx = {0}; // Initialize all to 0 for safety

// Uniform Buffer Object (UBO) for passing matrices to shaders
typedef struct {
    mat4 model; // Model matrix for object transformations
    mat4 view;  // View matrix from camera position/orientation
    mat4 proj;  // Projection matrix for perspective
} UBO;

// Find a memory type that matches requirements (e.g., host-visible)
uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkCtx.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i; // Return index of matching memory type
        }
    }
    printf("Failed to find suitable memory type!\n");
    exit(1); // Fatal error if no match found
}

// Update UBO with camera view and object rotation
void update_uniform_buffer(Camera* cam, float rotationAngle) {
    UBO ubo;
    glm_mat4_identity(ubo.model); // Reset model matrix to identity
    glm_rotate_y(ubo.model, glm_rad(rotationAngle), ubo.model); // Rotate objects around Y-axis
    glm_lookat(cam->pos, (vec3){cam->pos[0] + cam->front[0], cam->pos[1] + cam->front[1], cam->pos[2] + cam->front[2]}, cam->up, ubo.view);
    glm_perspective(glm_rad(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f, ubo.proj);

    void* data;
    vkMapMemory(vkCtx.device, vkCtx.uniformMemory, 0, sizeof(UBO), 0, &data);
    memcpy(data, &ubo, sizeof(UBO));
    vkUnmapMemory(vkCtx.device, vkCtx.uniformMemory);
}

// Initialize all Vulkan objects
void init_vulkan(SDL_Window* window) {
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Vulkan SDL3";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface"};
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, NULL, &vkCtx.instance) != VK_SUCCESS) {
        printf("Failed to create Vulkan instance\n");
        exit(1);
    }

    if (!SDL_Vulkan_CreateSurface(window, vkCtx.instance, NULL, &vkCtx.surface)) {
        printf("Failed to create Vulkan surface: %s\n", SDL_GetError());
        exit(1);
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkCtx.instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vkCtx.instance, &deviceCount, devices);
    vkCtx.physicalDevice = devices[0];
    free(devices);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkCtx.physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vkCtx.physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t graphicsFamily = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }
    free(queueFamilies);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(vkCtx.physicalDevice, &deviceCreateInfo, NULL, &vkCtx.device) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
        exit(1);
    }

    vkGetDeviceQueue(vkCtx.device, graphicsFamily, 0, &vkCtx.graphicsQueue);

    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = vkCtx.surface;
    swapchainInfo.minImageCount = 2;
    swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent.width = WIDTH;
    swapchainInfo.imageExtent.height = HEIGHT;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(vkCtx.device, &swapchainInfo, NULL, &vkCtx.swapchain) != VK_SUCCESS) {
        printf("Failed to create swapchain\n");
        exit(1);
    }

    vkGetSwapchainImagesKHR(vkCtx.device, vkCtx.swapchain, &vkCtx.imageCount, NULL);
    vkCtx.swapchainImages = malloc(vkCtx.imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(vkCtx.device, vkCtx.swapchain, &vkCtx.imageCount, vkCtx.swapchainImages);

    vkCtx.swapchainImageViews = malloc(vkCtx.imageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = vkCtx.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vkCtx.device, &viewInfo, NULL, &vkCtx.swapchainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image views\n");
            exit(1);
        }
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
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

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(vkCtx.device, &renderPassInfo, NULL, &vkCtx.renderPass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        exit(1);
    }

    vkCtx.swapchainFramebuffers = malloc(vkCtx.imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkCtx.swapchainImageViews[i];
        framebufferInfo.width = WIDTH;
        framebufferInfo.height = HEIGHT;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx.device, &framebufferInfo, NULL, &vkCtx.swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            exit(1);
        }
    }

    VkCommandPoolCreateInfo commandPoolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolInfo.queueFamilyIndex = graphicsFamily;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vkCtx.device, &commandPoolInfo, NULL, &vkCtx.commandPool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        exit(1);
    }

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(vkCtx.device, &allocInfo, &vkCtx.commandBuffer) != VK_SUCCESS) {
        printf("Failed to allocate command buffer\n");
        exit(1);
    }

    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vkCtx.device, &fenceInfo, NULL, &vkCtx.inFlightFence) != VK_SUCCESS) {
        printf("Failed to create synchronization objects\n");
        exit(1);
    }

    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(vkCtx.device, &layoutInfo, NULL, &vkCtx.descriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        exit(1);
    }

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(UBO);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &vkCtx.uniformBuffer) != VK_SUCCESS) {
        printf("Failed to create uniform buffer\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkCtx.device, vkCtx.uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo uboAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    uboAllocInfo.allocationSize = memRequirements.size;
    uboAllocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx.device, &uboAllocInfo, NULL, &vkCtx.uniformMemory) != VK_SUCCESS) {
        printf("Failed to allocate uniform buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx.device, vkCtx.uniformBuffer, vkCtx.uniformMemory, 0);

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.pPoolSizes = &poolSize;
    descriptorPoolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(vkCtx.device, &descriptorPoolInfo, NULL, &vkCtx.descriptorPool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool\n");
        exit(1);
    }

    VkDescriptorSetAllocateInfo allocSetInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocSetInfo.descriptorPool = vkCtx.descriptorPool;
    allocSetInfo.descriptorSetCount = 1;
    allocSetInfo.pSetLayouts = &vkCtx.descriptorSetLayout;

    if (vkAllocateDescriptorSets(vkCtx.device, &allocSetInfo, &vkCtx.descriptorSet) != VK_SUCCESS) {
        printf("Failed to allocate descriptor sets\n");
        exit(1);
    }

    VkDescriptorBufferInfo bufferDescriptorInfo = {};
    bufferDescriptorInfo.buffer = vkCtx.uniformBuffer;
    bufferDescriptorInfo.offset = 0;
    bufferDescriptorInfo.range = sizeof(UBO);

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = vkCtx.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferDescriptorInfo;

    vkUpdateDescriptorSets(vkCtx.device, 1, &descriptorWrite, 0, NULL);
}

// Create triangle vertex buffer
void create_triangle() {
    if (vkCtx.triangle.exists) {
        printf("Triangle already exists, skipping creation\n");
        return;
    }

    float vertices[] = {
        0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // Top vertex: red
       -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // Bottom-left: green
        0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // Bottom-right: blue
    };

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &vkCtx.triangle.buffer) != VK_SUCCESS) {
        printf("Failed to create triangle vertex buffer\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkCtx.device, vkCtx.triangle.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &vkCtx.triangle.memory) != VK_SUCCESS) {
        printf("Failed to allocate triangle vertex buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx.device, vkCtx.triangle.buffer, vkCtx.triangle.memory, 0);

    void* data;
    vkMapMemory(vkCtx.device, vkCtx.triangle.memory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vkCtx.device, vkCtx.triangle.memory);

    vkCtx.triangle.vertexCount = 3;
    vkCtx.triangle.exists = true;
    printf("Triangle created\n");
}

// Destroy triangle vertex buffer
void destroy_triangle() {
    if (!vkCtx.triangle.exists) {
        printf("Triangle does not exist, skipping destruction\n");
        return;
    }

    vkDeviceWaitIdle(vkCtx.device); // Ensure GPU is idle before destroying
    vkDestroyBuffer(vkCtx.device, vkCtx.triangle.buffer, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.triangle.memory, NULL);
    vkCtx.triangle.buffer = VK_NULL_HANDLE;
    vkCtx.triangle.memory = VK_NULL_HANDLE;
    vkCtx.triangle.vertexCount = 0;
    vkCtx.triangle.exists = false;
    printf("Triangle destroyed\n");
}

// Create cube vertex buffer
void create_cube() {
    if (vkCtx.cube.exists) {
        printf("Cube already exists, skipping creation\n");
        return;
    }

    // Cube vertices: 8 corners, expanded to 36 for 12 triangles (triangle list)
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 0
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 1
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 2
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 0
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 2
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, // 3
        // Back face
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, // 4
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, // 5
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, // 6
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, // 4
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, // 6
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f, // 7
        // Left face
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 0
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f, // 4
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f, // 7
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 0
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f, // 7
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, // 3
        // Right face
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 1
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, // 5
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, // 6
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 1
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, // 6
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 2
        // Top face
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, // 3
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 2
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, // 6
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f, // 3
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f, // 6
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f, // 7
        // Bottom face
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 0
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 1
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, // 5
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 0
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f, // 5
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f  // 4
    };

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &vkCtx.cube.buffer) != VK_SUCCESS) {
        printf("Failed to create cube vertex buffer\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkCtx.device, vkCtx.cube.buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &vkCtx.cube.memory) != VK_SUCCESS) {
        printf("Failed to allocate cube vertex buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx.device, vkCtx.cube.buffer, vkCtx.cube.memory, 0);

    void* data;
    vkMapMemory(vkCtx.device, vkCtx.cube.memory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vkCtx.device, vkCtx.cube.memory);

    vkCtx.cube.vertexCount = 36; // 36 vertices for 12 triangles
    vkCtx.cube.exists = true;
    printf("Cube created\n");
}

// Destroy cube vertex buffer
void destroy_cube() {
    if (!vkCtx.cube.exists) {
        printf("Cube does not exist, skipping destruction\n");
        return;
    }

    vkDeviceWaitIdle(vkCtx.device); // Ensure GPU is idle before destroying
    vkDestroyBuffer(vkCtx.device, vkCtx.cube.buffer, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.cube.memory, NULL);
    vkCtx.cube.buffer = VK_NULL_HANDLE;
    vkCtx.cube.memory = VK_NULL_HANDLE;
    vkCtx.cube.vertexCount = 0;
    vkCtx.cube.exists = false;
    printf("Cube destroyed\n");
}

// Create graphics pipeline with shaders
void create_pipeline() {
    FILE* vertFile = fopen("vert.spv", "rb");
    if (!vertFile) {
        printf("Failed to open vert.spv\n");
        exit(1);
    }
    fseek(vertFile, 0, SEEK_END);
    long vertSize = ftell(vertFile);
    fseek(vertFile, 0, SEEK_SET);
    char* vertShaderCode = malloc(vertSize);
    fread(vertShaderCode, 1, vertSize, vertFile);
    fclose(vertFile);

    FILE* fragFile = fopen("frag.spv", "rb");
    if (!fragFile) {
        printf("Failed to open frag.spv\n");
        exit(1);
    }
    fseek(fragFile, 0, SEEK_END);
    long fragSize = ftell(fragFile);
    fseek(fragFile, 0, SEEK_SET);
    char* fragShaderCode = malloc(fragSize);
    fread(fragShaderCode, 1, fragSize, fragFile);
    fclose(fragFile);

    VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertShaderInfo.codeSize = vertSize;
    vertShaderInfo.pCode = (uint32_t*)vertShaderCode;
    VkShaderModule vertModule;
    if (vkCreateShaderModule(vkCtx.device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
        printf("Failed to create vertex shader module\n");
        exit(1);
    }

    VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragShaderInfo.codeSize = fragSize;
    fragShaderInfo.pCode = (uint32_t*)fragShaderCode;
    VkShaderModule fragModule;
    if (vkCreateShaderModule(vkCtx.device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
        printf("Failed to create fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
    };

    VkVertexInputBindingDescription bindingDesc = {0, 6 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrDesc[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)}
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {WIDTH, HEIGHT}};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vkCtx.descriptorSetLayout;

    if (vkCreatePipelineLayout(vkCtx.device, &pipelineLayoutInfo, NULL, &vkCtx.pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = vkCtx.pipelineLayout;
    pipelineInfo.renderPass = vkCtx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkCtx.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx.graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vkCtx.device, fragModule, NULL);
    vkDestroyShaderModule(vkCtx.device, vertModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
}

// Record commands for rendering one frame with multiple objects
void record_command_buffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(vkCtx.commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin command buffer\n");
        exit(1);
    }

    VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = vkCtx.renderPass;
    renderPassInfo.framebuffer = vkCtx.swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = (VkExtent2D){WIDTH, HEIGHT};
    VkClearValue clearColor = {{{0.5f, 0.5f, 0.5f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.graphicsPipeline);
    vkCmdBindDescriptorSets(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.pipelineLayout, 0, 1, &vkCtx.descriptorSet, 0, NULL);

    VkDeviceSize offsets[] = {0};
    if (vkCtx.triangle.exists) {
        vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.triangle.buffer, offsets);
        vkCmdDraw(vkCtx.commandBuffer, vkCtx.triangle.vertexCount, 1, 0, 0);
    }
    if (vkCtx.cube.exists) {
        vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.cube.buffer, offsets);
        vkCmdDraw(vkCtx.commandBuffer, vkCtx.cube.vertexCount, 1, 0, 0);
    }

    vkCmdEndRenderPass(vkCtx.commandBuffer);
    if (vkEndCommandBuffer(vkCtx.commandBuffer) != VK_SUCCESS) {
        printf("Failed to end command buffer\n");
        exit(1);
    }
}

// Reset camera to initial state
void reset_camera(Camera* cam) {
    cam->pos[0] = 0.0f; cam->pos[1] = 0.0f; cam->pos[2] = 3.0f;
    cam->front[0] = 0.0f; cam->front[1] = 0.0f; cam->front[2] = -1.0f;
    cam->up[0] = 0.0f; cam->up[1] = 1.0f; cam->up[2] = 0.0f;
    cam->yaw = -90.0f;
    cam->pitch = 0.0f;
    printf("Camera reset: Pos [0, 0, 3], Yaw -90, Pitch 0\n");
}

// Update camera based on input and time
void update_camera(Camera* cam, SDL_Event* event, bool* mouseCaptured, SDL_Window* window, Uint64 deltaTime) {
    const float speed = 2.5f;
    float moveSpeed = speed * (deltaTime / 1000.0f);

    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.key) {
            case SDLK_W: {
                vec3 move; glm_vec3_scale(cam->front, moveSpeed, move); glm_vec3_add(cam->pos, move, cam->pos); 
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_S: {
                vec3 move; glm_vec3_scale(cam->front, -moveSpeed, move); glm_vec3_add(cam->pos, move, cam->pos); 
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_A: {
                vec3 left; glm_vec3_cross(cam->front, cam->up, left); glm_vec3_normalize(left); glm_vec3_scale(left, -moveSpeed, left); glm_vec3_add(cam->pos, left, cam->pos); 
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_D: {
                vec3 right; glm_vec3_cross(cam->front, cam->up, right); glm_vec3_normalize(right); glm_vec3_scale(right, moveSpeed, right); glm_vec3_add(cam->pos, right, cam->pos); 
                printf("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_ESCAPE: {
                *mouseCaptured = false;
                if (!SDL_SetWindowRelativeMouseMode(window, false)) {
                    printf("Failed to disable relative mouse mode: %s\n", SDL_GetError());
                }
                break;
            }
        }
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        *mouseCaptured = true;
        if (!SDL_SetWindowRelativeMouseMode(window, true)) {
            printf("Failed to enable relative mouse mode: %s\n", SDL_GetError());
        }
    }
    if (*mouseCaptured && event->type == SDL_EVENT_MOUSE_MOTION) {
        float sensitivity = 0.1f;
        cam->yaw += event->motion.xrel * sensitivity;
        cam->pitch += event->motion.yrel * sensitivity;
        if (cam->pitch > 89.0f) cam->pitch = 89.0f;
        if (cam->pitch < -89.0f) cam->pitch = -89.0f;

        vec3 front;
        front[0] = cosf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
        front[1] = sinf(glm_rad(cam->pitch));
        front[2] = sinf(glm_rad(cam->yaw)) * cosf(glm_rad(cam->pitch));
        glm_vec3_normalize_to(front, cam->front);
        printf("Camera Yaw: %.2f, Pitch: %.2f, Front: [%.2f, %.2f, %.2f]\n", cam->yaw, cam->pitch, cam->front[0], cam->front[1], cam->front[2]);
    }
}

// Main entry point
int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    init_vulkan(window);
    create_pipeline();

    Camera cam = {{0.0f, 0.0f, 3.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, -90.0f, 0.0f};
    bool mouseCaptured = false;
    bool running = true;
    bool rotateObjects = false;
    float rotationAngle = 0.0f;
    Uint64 lastTime = SDL_GetTicks();
    SDL_Event event;

    while (running) {
        Uint64 currentTime = SDL_GetTicks();
        Uint64 deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            update_camera(&cam, &event, &mouseCaptured, window, deltaTime);

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB) {
                rotateObjects = !rotateObjects;
                printf("Object rotation %s\n", rotateObjects ? "enabled" : "disabled");
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_1) {
                rotationAngle = 0.0f;
                printf("Object rotation reset to 0\n");
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_2) {
                reset_camera(&cam);
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_4) {
                if (!vkCtx.triangle.exists) {
                    create_triangle();
                } else {
                    destroy_triangle();
                }
            }

            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_5) {
                if (!vkCtx.cube.exists) {
                    create_cube();
                } else {
                    destroy_cube();
                }
            }
        }

        if (rotateObjects) {
            rotationAngle += 90.0f * (deltaTime / 1000.0f);
            if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        }

        update_uniform_buffer(&cam, rotationAngle);

        vkWaitForFences(vkCtx.device, 1, &vkCtx.inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vkCtx.device, 1, &vkCtx.inFlightFence);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkCtx.device, vkCtx.swapchain, UINT64_MAX, vkCtx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            printf("Failed to acquire next image: %d\n", result);
            exit(1);
        }

        vkResetCommandBuffer(vkCtx.commandBuffer, 0);
        record_command_buffer(imageIndex);

        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkSemaphore waitSemaphores[] = {vkCtx.imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCtx.commandBuffer;
        VkSemaphore signalSemaphores[] = {vkCtx.renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, vkCtx.inFlightFence) != VK_SUCCESS) {
            printf("Failed to submit draw command buffer\n");
            exit(1);
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkCtx.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        if (vkQueuePresentKHR(vkCtx.graphicsQueue, &presentInfo) != VK_SUCCESS) {
            printf("Failed to present image\n");
            exit(1);
        }
    }

    vkDeviceWaitIdle(vkCtx.device);

    if (vkCtx.triangle.exists) destroy_triangle();
    if (vkCtx.cube.exists) destroy_cube();

    vkDestroySemaphore(vkCtx.device, vkCtx.renderFinishedSemaphore, NULL);
    vkDestroySemaphore(vkCtx.device, vkCtx.imageAvailableSemaphore, NULL);
    vkDestroyFence(vkCtx.device, vkCtx.inFlightFence, NULL);
    vkDestroyCommandPool(vkCtx.device, vkCtx.commandPool, NULL);
    vkDestroyPipeline(vkCtx.device, vkCtx.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(vkCtx.device, vkCtx.pipelineLayout, NULL);
    vkDestroyDescriptorPool(vkCtx.device, vkCtx.descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(vkCtx.device, vkCtx.descriptorSetLayout, NULL);
    vkDestroyBuffer(vkCtx.device, vkCtx.uniformBuffer, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.uniformMemory, NULL);
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        vkDestroyFramebuffer(vkCtx.device, vkCtx.swapchainFramebuffers[i], NULL);
        vkDestroyImageView(vkCtx.device, vkCtx.swapchainImageViews[i], NULL);
    }
    free(vkCtx.swapchainFramebuffers);
    free(vkCtx.swapchainImages);
    free(vkCtx.swapchainImageViews);
    vkDestroyRenderPass(vkCtx.device, vkCtx.renderPass, NULL);
    vkDestroySwapchainKHR(vkCtx.device, vkCtx.swapchain, NULL);
    vkDestroyDevice(vkCtx.device, NULL);
    vkDestroySurfaceKHR(vkCtx.instance, vkCtx.surface, NULL);
    vkDestroyInstance(vkCtx.instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}