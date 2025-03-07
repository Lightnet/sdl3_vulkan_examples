// vsdl_pipeline.cpp
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <SDL3/SDL_log.h>
#include <fstream>
#include <vector>
#include "vsdl_pipeline.h"
#include "vsdl_types.h"
#include "vsdl_mesh.h"

static std::vector<char> readFile(const char* filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open shader file: %s", filename);
        return {};
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

bool create_pipeline(VSDL_Context& ctx) {
    SDL_Log("Creating pipeline");

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = ctx.swapchainImageFormat;
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

    if (vkCreateRenderPass(ctx.device, &renderPassInfo, nullptr, &ctx.renderPass) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create render pass");
        return false;
    }
    SDL_Log("Render pass created");

    ctx.framebuffers.resize(ctx.swapchainImageViews.size());
    for (size_t i = 0; i < ctx.swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {ctx.swapchainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = ctx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = ctx.swapchainExtent.width;
        framebufferInfo.height = ctx.swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &ctx.framebuffers[i]) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create framebuffer %zu", i);
            return false;
        }
    }
    SDL_Log("Framebuffers created (count: %zu)", ctx.framebuffers.size());

    if (!create_vertex_buffer(ctx)) {
        return false;
    }

    // auto vertShaderCode = readFile("vertex.spv");
    // auto fragShaderCode = readFile("fragment.spv");
    auto vertShaderCode = readFile("shaders/tri.vert.spv");
    auto fragShaderCode = readFile("shaders/tri.frag.spv");
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        return false;
    }

    VkShaderModule vertShaderModule, fragShaderModule;
    VkShaderModuleCreateInfo vertInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertInfo.codeSize = vertShaderCode.size();
    vertInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
    if (vkCreateShaderModule(ctx.device, &vertInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex shader module");
        return false;
    }

    VkShaderModuleCreateInfo fragInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragInfo.codeSize = fragShaderCode.size();
    fragInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
    if (vkCreateShaderModule(ctx.device, &fragInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create fragment shader module");
        vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", nullptr}
    };

    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex); // Line 119
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDesc = {};
    attributeDesc.binding = 0;
    attributeDesc.location = 0;
    attributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
    attributeDesc.offset = offsetof(Vertex, pos); // Line 126

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0.0f, 0.0f, (float)ctx.swapchainExtent.width, (float)ctx.swapchainExtent.height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, ctx.swapchainExtent};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (vkCreatePipelineLayout(ctx.device, &pipelineLayoutInfo, nullptr, &ctx.pipelineLayout) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create pipeline layout");
        vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);
        vkDestroyShaderModule(ctx.device, fragShaderModule, nullptr);
        return false;
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
    pipelineInfo.layout = ctx.pipelineLayout;
    pipelineInfo.renderPass = ctx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ctx.graphicsPipeline) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create graphics pipeline");
        vkDestroyPipelineLayout(ctx.device, ctx.pipelineLayout, nullptr);
        vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);
        vkDestroyShaderModule(ctx.device, fragShaderModule, nullptr);
        return false;
    }
    SDL_Log("Graphics pipeline created");

    vkDestroyShaderModule(ctx.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(ctx.device, fragShaderModule, nullptr);

    return true;
}