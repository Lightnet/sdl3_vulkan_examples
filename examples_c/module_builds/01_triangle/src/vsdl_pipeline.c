#include "vsdl_pipeline.h"
#include <stdio.h>

static VkShaderModule create_shader_module(const char* filename) {
  char path[256];
  snprintf(path, sizeof(path), "shaders/%s", filename);  // e.g., "shaders/triangle.vert.spv"
  FILE* file = fopen(path, "rb");
  if (!file) {
      LOG_ERROR("Failed to open shader file: %s", path);
      exit(1);  // Or handle gracefully
  }
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* code = malloc(size);
  fread(code, 1, size, file);
  fclose(file);

  VkShaderModuleCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = size,
      .pCode = (uint32_t*)code
  };
  VkShaderModule module;
  if (vkCreateShaderModule(g_context.device, &createInfo, NULL, &module) != VK_SUCCESS) {
      LOG_ERROR("Failed to create shader module: %s", filename);
      free(code);
      exit(1);
  }
  free(code);
  return module;
}

int vsdl_pipeline_create(void) {
    LOG("Creating pipeline");

    VkShaderModule vertShader = create_shader_module("triangle.vert.spv");
    VkShaderModule fragShader = create_shader_module("triangle.frag.spv");
    if (!vertShader || !fragShader) {
        LOG_ERROR("Failed to create shader modules");
        return 1;
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vertShader, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = fragShader, .pName = "main" }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    VkViewport viewport = { .x = 0.0f, .y = 0.0f, .width = 800.0f, .height = 600.0f, .minDepth = 0.0f, .maxDepth = 1.0f };
    VkRect2D scissor = { .offset = {0, 0}, .extent = {800, 600} };
    VkPipelineViewportStateCreateInfo viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissor };
    VkPipelineRasterizationStateCreateInfo rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .polygonMode = VK_POLYGON_MODE_FILL, .lineWidth = 1.0f, .cullMode = VK_CULL_MODE_NONE };
    VkPipelineMultisampleStateCreateInfo multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT };
    VkPipelineColorBlendAttachmentState colorBlendAttachment = { .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, .blendEnable = VK_FALSE };
    VkPipelineColorBlendStateCreateInfo colorBlending = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &colorBlendAttachment };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &(VkAttachmentDescription){ .format = VK_FORMAT_B8G8R8A8_SRGB, .samples = VK_SAMPLE_COUNT_1_BIT, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, .storeOp = VK_ATTACHMENT_STORE_OP_STORE, .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
        .subpassCount = 1,
        .pSubpasses = &(VkSubpassDescription){ .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, .colorAttachmentCount = 1, .pColorAttachments = &(VkAttachmentReference){ .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } }
    };
    if (vkCreateRenderPass(g_context.device, &renderPassInfo, NULL, &g_context.renderPass) != VK_SUCCESS) return 1;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    if (vkCreatePipelineLayout(g_context.device, &pipelineLayoutInfo, NULL, &g_context.pipelineLayout) != VK_SUCCESS) return 1;

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
        .layout = g_context.pipelineLayout,
        .renderPass = g_context.renderPass,
        .subpass = 0
    };
    if (vkCreateGraphicsPipelines(g_context.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &g_context.graphicsPipeline) != VK_SUCCESS) return 1;

    g_context.framebuffers = malloc(g_context.swapchainImageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < g_context.swapchainImageCount; i++) {
        VkFramebufferCreateInfo fbInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = g_context.renderPass,
            .attachmentCount = 1,
            .pAttachments = &g_context.swapchainImageViews[i],
            .width = 800,
            .height = 600,
            .layers = 1
        };
        if (vkCreateFramebuffer(g_context.device, &fbInfo, NULL, &g_context.framebuffers[i]) != VK_SUCCESS) return 1;
    }

    VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = 0 };
    if (vkCreateCommandPool(g_context.device, &poolInfo, NULL, &g_context.commandPool) != VK_SUCCESS) return 1;

    VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = g_context.commandPool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    if (vkAllocateCommandBuffers(g_context.device, &allocInfo, &g_context.commandBuffer) != VK_SUCCESS) return 1;

    vkDestroyShaderModule(g_context.device, fragShader, NULL);
    vkDestroyShaderModule(g_context.device, vertShader, NULL);
    return 0;
}