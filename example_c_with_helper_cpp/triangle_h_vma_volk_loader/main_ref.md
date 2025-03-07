#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Link to deps.cpp for Volk initialization
extern void init_volk(void);

// Error handling macro for Vulkan calls
#define VK_CHECK(call) do { \
    VkResult result = (call); \
    if (result != VK_SUCCESS) { \
        printf("[ERROR] Vulkan call failed at %s:%d: %d\n", __FILE__, __LINE__, result); \
        exit(1); \
    } \
} while(0)

// Vertex structure (position and color)
typedef struct {
    float pos[2];
    float color[3];
} Vertex;

// Hardcoded triangle vertices
static const Vertex vertices[] = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // Top, Red
    {{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}},  // Bottom Right, Green
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}   // Bottom Left, Blue
};

int main(int argc, char* argv[]) {
    /* --- Initialization --- */
    // Initialize SDL3 with video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[ERROR] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create a window with Vulkan support
    SDL_Window* window = SDL_CreateWindow("Vulkan Triangle", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("[ERROR] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("[INFO] Window created successfully\n");

    // Initialize Volk to load Vulkan functions
    init_volk();
    printf("[INFO] Volk initialized\n");

    /* --- Vulkan Instance --- */
    // Get required Vulkan instance extensions from SDL
    Uint32 ext_count;
    if (!SDL_Vulkan_GetInstanceExtensions(&ext_count, NULL)) {
        printf("[ERROR] SDL_Vulkan_GetInstanceExtensions (count) failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    const char** ext_names = malloc(ext_count * sizeof(const char*));
    if (!SDL_Vulkan_GetInstanceExtensions(&ext_count, ext_names)) {
        printf("[ERROR] SDL_Vulkan_GetInstanceExtensions (names) failed: %s\n", SDL_GetError());
        free(ext_names);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create Vulkan instance
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = ext_count,
        .ppEnabledExtensionNames = ext_names,
        .enabledLayerCount = 0  // Add validation layers here (e.g., "VK_LAYER_KHRONOS_validation")
    };
    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instance_info, NULL, &instance));
    free(ext_names);
    printf("[INFO] Vulkan instance created\n");

    // Create Vulkan surface via SDL
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &surface)) {
        printf("[ERROR] SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        vkDestroyInstance(instance, NULL);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* --- Physical Device and Queue Family --- */
    // Pick a physical device
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (device_count == 0) {
        printf("[ERROR] No Vulkan-capable devices found\n");
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    VkPhysicalDevice* devices = malloc(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
    VkPhysicalDevice physical_device = devices[0];  // Pick first device (improve selection later)
    free(devices);

    // Find queue family with graphics support
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
    uint32_t graphics_queue_family = UINT32_MAX;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if (present_support) {
                graphics_queue_family = i;
                break;
            }
        }
    }
    free(queue_families);
    if (graphics_queue_family == UINT32_MAX) {
        printf("[ERROR] No suitable queue family found\n");
        vkDestroySurfaceKHR(instance, surface, NULL);
        vkDestroyInstance(instance, NULL);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* --- Logical Device and Queue --- */
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphics_queue_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };
    const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = device_extensions
    };
    VkDevice device;
    VK_CHECK(vkCreateDevice(physical_device, &device_info, NULL, &device));
    VkQueue graphics_queue;
    vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);

    /* --- Swapchain --- */
    VkSurfaceCapabilitiesKHR surface_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps);
    VkExtent2D extent = surface_caps.currentExtent;
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface_caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .queueFamilyIndexCount = 0
    };
    VkSwapchainKHR swapchain;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_info, NULL, &swapchain));
    uint32_t image_count;
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, NULL);
    VkImage* swapchain_images = malloc(image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchain, &image_count, swapchain_images);

    /* --- Render Pass --- */
    VkAttachmentDescription color_attachment = {
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };
    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref
    };
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };
    VkRenderPass render_pass;
    VK_CHECK(vkCreateRenderPass(device, &render_pass_info, NULL, &render_pass));

    /* --- Pipeline --- */
    // Load shaders (assumes shaders are in build/Debug/shaders/)
    FILE* vert_file = fopen("Debug/shaders/triangle.vert.spv", "rb");
    if (!vert_file) {
        printf("[ERROR] Failed to open vertex shader\n");
        exit(1);
    }
    fseek(vert_file, 0, SEEK_END);
    size_t vert_size = ftell(vert_file);
    rewind(vert_file);
    char* vert_code = malloc(vert_size);
    fread(vert_code, 1, vert_size, vert_file);
    fclose(vert_file);

    FILE* frag_file = fopen("Debug/shaders/triangle.frag.spv", "rb");
    if (!frag_file) {
        printf("[ERROR] Failed to open fragment shader\n");
        free(vert_code);
        exit(1);
    }
    fseek(frag_file, 0, SEEK_END);
    size_t frag_size = ftell(frag_file);
    rewind(frag_file);
    char* frag_code = malloc(frag_size);
    fread(frag_code, 1, frag_size, frag_file);
    fclose(frag_file);

    VkShaderModuleCreateInfo vert_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vert_size,
        .pCode = (uint32_t*)vert_code
    };
    VkShaderModule vert_module;
    VK_CHECK(vkCreateShaderModule(device, &vert_module_info, NULL, &vert_module));
    VkShaderModuleCreateInfo frag_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = frag_size,
        .pCode = (uint32_t*)frag_code
    };
    VkShaderModule frag_module;
    VK_CHECK(vkCreateShaderModule(device, &frag_module_info, NULL, &frag_module));

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main"
        }
    };

    VkVertexInputBindingDescription binding_desc = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription attrib_descs[2] = {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, pos) },
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color) }
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = attrib_descs
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    VkViewport viewport = { 0.0f, 0.0f, (float)extent.width, (float)extent.height, 0.0f, 1.0f };
    VkRect2D scissor = { {0, 0}, extent };
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f
    };
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .renderPass = render_pass,
        .subpass = 0
    };
    VkPipeline pipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline));
    free(vert_code);
    free(frag_code);

    /* --- Vertex Buffer --- */
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(vertices),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VkBuffer vertex_buffer;
    VK_CHECK(vkCreateBuffer(device, &buffer_info, NULL, &vertex_buffer));
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, vertex_buffer, &mem_reqs);
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
    uint32_t memory_type_index = UINT32_MAX;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((mem_reqs.memoryTypeBits & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            memory_type_index = i;
            break;
        }
    }
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = memory_type_index
    };
    VkDeviceMemory vertex_memory;
    VK_CHECK(vkAllocateMemory(device, &alloc_info, NULL, &vertex_memory));
    void* data;
    VK_CHECK(vkMapMemory(device, vertex_memory, 0, sizeof(vertices), 0, &data));
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(device, vertex_memory);
    VK_CHECK(vkBindBufferMemory(device, vertex_buffer, vertex_memory, 0));

    /* --- Framebuffers --- */
    VkImageView* image_views = malloc(image_count * sizeof(VkImageView));
    for (uint32_t i = 0; i < image_count; i++) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .components = { VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 }
        };
        VK_CHECK(vkCreateImageView(device, &view_info, NULL, &image_views[i]));
    }
    VkFramebuffer* framebuffers = malloc(image_count * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < image_count; i++) {
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &image_views[i],
            .width = extent.width,
            .height = extent.height,
            .layers = 1
        };
        VK_CHECK(vkCreateFramebuffer(device, &fb_info, NULL, &framebuffers[i]));
    }

    /* --- Command Buffers --- */
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphics_queue_family
    };
    VkCommandPool command_pool;
    VK_CHECK(vkCreateCommandPool(device, &pool_info, NULL, &command_pool));
    VkCommandBufferAllocateInfo cmd_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = image_count
    };
    VkCommandBuffer* command_buffers = malloc(image_count * sizeof(VkCommandBuffer));
    VK_CHECK(vkAllocateCommandBuffers(device, &cmd_buffer_info, command_buffers));
    for (uint32_t i = 0; i < image_count; i++) {
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        };
        VK_CHECK(vkBeginCommandBuffer(command_buffers[i], &begin_info));
        VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        VkRenderPassBeginInfo rp_begin_info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass,
            .framebuffer = framebuffers[i],
            .renderArea = { {0, 0}, extent },
            .clearValueCount = 1,
            .pClearValues = &clear_color
        };
        vkCmdBeginRenderPass(command_buffers[i], &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, &vertex_buffer, offsets);
        vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffers[i]);
        VK_CHECK(vkEndCommandBuffer(command_buffers[i]));
    }

    /* --- Synchronization --- */
    VkSemaphoreCreateInfo semaphore_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore image_available_semaphore, render_finished_semaphore;
    VK_CHECK(vkCreateSemaphore(device, &semaphore_info, NULL, &image_available_semaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphore_info, NULL, &render_finished_semaphore));

    /* --- Main Loop --- */
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:  // SDL3 event type
                    switch (event.key.key) {  // SDL3: event.key.key (not keysym.sym)
                        case SDLK_LEFT:
                            printf("[INFO] Left key pressed\n");
                            break;
                        case SDLK_RIGHT:
                            printf("[INFO] Right key pressed\n");
                            break;
                        case SDLK_ESCAPE:
                            running = false;
                            break;
                    }
                    break;
            }
        }

        // Acquire next image
        uint32_t image_index;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

        // Submit command buffer
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &image_available_semaphore,
            .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffers[image_index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_finished_semaphore
        };
        VK_CHECK(vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

        // Present
        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index
        };
        VK_CHECK(vkQueuePresentKHR(graphics_queue, &present_info));
        vkQueueWaitIdle(graphics_queue);  // Simple sync (improve with fences later)
    }

    /* --- Cleanup --- */
    vkDestroySemaphore(device, image_available_semaphore, NULL);
    vkDestroySemaphore(device, render_finished_semaphore, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);
    free(command_buffers);
    for (uint32_t i = 0; i < image_count; i++) vkDestroyFramebuffer(device, framebuffers[i], NULL);
    free(framebuffers);
    for (uint32_t i = 0; i < image_count; i++) vkDestroyImageView(device, image_views[i], NULL);
    free(image_views);
    vkDestroyBuffer(device, vertex_buffer, NULL);
    vkDestroyMemory(device, vertex_memory, NULL);
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyShaderModule(device, vert_module, NULL);
    vkDestroyShaderModule(device, frag_module, NULL);
    vkDestroyRenderPass(device, render_pass, NULL);
    free(swapchain_images);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("[INFO] Application exited cleanly\n");
    return 0;
}

/* --- Notes --- */
/*
 * 1. SDL3 API: Uses event.key.key (not keysym.sym) per SDL3's updated event structure.
 * 2. Vulkan Pipeline: Simplified triangle rendering with hardcoded vertices.
 * 3. Memory Management: Manual allocation for simplicity; consider VMA for production.
 * 4. Synchronization: Basic semaphores; add fences for better performance.
 * 5. Error Handling: VK_CHECK macro logs errors with file/line for debugging.
 */

/* --- Troubleshooting Guide --- */
/*
 * 1. Window Doesn't Open:
 *    - Check SDL_Init and SDL_CreateWindow errors in run_log.txt.
 *    - Ensure SDL3.dll is in build/Debug/.
 * 2. Vulkan Fails:
 *    - "volkInitialize failed": Ensure vulkan-1.dll is in build/Debug/ or PATH.
 *    - "No devices found": Verify Vulkan drivers (e.g., GPU driver update).
 * 3. Black Screen:
 *    - Shaders missing: Check build_log.txt for glslc errors; ensure shaders/ directory exists.
 *    - Pipeline issue: Verify vkCreateGraphicsPipelines result in debug output.
 * 4. Events Not Working:
 *    - Ensure event.key.key is used (SDL3-specific).
 *    - Test with printf in event loop.
 * 5. Crash on Exit:
 *    - Check cleanup order (matches creation order in reverse).
 *    - Enable validation layers for detailed errors (add to instance_info).
 */

/* --- Build Order Explanation --- */
/*
 * 1. CMake Configure (cmake -S . -B build):
 *    - Downloads CPM, fetches SDL3, Vulkan-Loader, etc.
 *    - Sets up build rules for shaders and executable.
 * 2. Shader Compilation (cmake --build):
 *    - glslc compiles triangle.vert/frag to .spv files in build/Debug/shaders/.
 * 3. Dependency Build:
 *    - Vulkan-Loader builds vulkan-1.dll/lib.
 *    - SDL3 builds SDL3.dll/lib.
 *    - Volk and VMA are header-only (no separate build).
 * 4. vk_sdl_test Build:
 *    - Compiles main.c and deps.cpp.
 *    - Links against vulkan-1.lib, SDL3.lib, etc.
 *    - Copies shaders to build/Debug/shaders/.
 * 5. Run (./run.bat):
 *    - Executes build/Debug/vk_sdl_test.exe, logs to run_log.txt.
 */