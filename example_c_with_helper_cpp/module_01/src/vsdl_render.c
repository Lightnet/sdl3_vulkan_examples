#include "vsdl_render.h"
#include "vsdl_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <spirv_cross_c.h>

#define WIDTH 800  // Define here or pass via vkCtx if it becomes dynamic
#define HEIGHT 600 // Define here or pass via vkCtx if it becomes dynamic

void vsdl_create_pipeline(VulkanContext* vkCtx) { // Match declaration
  vsdl_log("Attempting to load shaders...\n");

  FILE* vertFile = fopen("vert.spv", "rb");
  if (!vertFile) {
      vsdl_log("Failed to open vert.spv - ensure it’s in the Debug directory\n");
      exit(1);
  }
  fseek(vertFile, 0, SEEK_END);
  long vertSize = ftell(vertFile);
  fseek(vertFile, 0, SEEK_SET);
  char* vertShaderCode = malloc(vertSize);
  fread(vertShaderCode, 1, vertSize, vertFile);
  fclose(vertFile);
  vsdl_log("Vertex shader loaded successfully\n");

  FILE* fragFile = fopen("frag.spv", "rb");
  if (!fragFile) {
      vsdl_log("Failed to open frag.spv - ensure it’s in the Debug directory\n");
      exit(1);
  }
  fseek(fragFile, 0, SEEK_END);
  long fragSize = ftell(fragFile);
  fseek(fragFile, 0, SEEK_SET);
  char* fragShaderCode = malloc(fragSize);
  fread(fragShaderCode, 1, fragSize, fragFile);
  fclose(fragFile);
  vsdl_log("Fragment shader loaded successfully\n");

  VkVertexInputAttributeDescription* attrDesc = NULL;
  uint32_t attrCount = 0;
  uint32_t stride = 0;
  vsdl_reflect_vertex_inputs(vertShaderCode, vertSize, &attrDesc, &attrCount, &stride);

  VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  vertShaderInfo.codeSize = vertSize;
  vertShaderInfo.pCode = (uint32_t*)vertShaderCode;
  VkShaderModule vertModule;
  if (vkCreateShaderModule(vkCtx->device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
      vsdl_log("Failed to create vertex shader module\n");
      exit(1);
  }

  VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  fragShaderInfo.codeSize = fragSize;
  fragShaderInfo.pCode = (uint32_t*)fragShaderCode;
  VkShaderModule fragModule;
  if (vkCreateShaderModule(vkCtx->device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
      vsdl_log("Failed to create fragment shader module\n");
      exit(1);
  }

  VkPipelineShaderStageCreateInfo shaderStages[] = {
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
  };

  VkVertexInputBindingDescription bindingDesc = {0, stride, VK_VERTEX_INPUT_RATE_VERTEX};
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
  vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkViewport viewport = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, 1.0f}; // Use defined constants
  VkRect2D scissor = {{0, 0}, {WIDTH, HEIGHT}};
  VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.lineWidth = 1.0f;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &vkCtx->descriptorSetLayout;

  if (vkCreatePipelineLayout(vkCtx->device, &pipelineLayoutInfo, NULL, &vkCtx->pipelineLayout) != VK_SUCCESS) {
      vsdl_log("Failed to create pipeline layout\n");
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
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = vkCtx->pipelineLayout;
  pipelineInfo.renderPass = vkCtx->renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(vkCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx->graphicsPipeline) != VK_SUCCESS) {
      vsdl_log("Failed to create graphics pipeline\n");
      exit(1);
  }

  vkDestroyShaderModule(vkCtx->device, fragModule, NULL);
  vkDestroyShaderModule(vkCtx->device, vertModule, NULL);
  free(vertShaderCode);
  free(fragShaderCode);
  free(attrDesc);
  vsdl_log("Graphics pipeline created successfully\n");
}

void vsdl_record_command_buffer(VulkanContext* vkCtx, uint32_t imageIndex) { // Match declaration
  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  if (vkBeginCommandBuffer(vkCtx->commandBuffer, &beginInfo) != VK_SUCCESS) {
      vsdl_log("Failed to begin command buffer\n");
      exit(1);
  }

  VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  renderPassInfo.renderPass = vkCtx->renderPass;
  renderPassInfo.framebuffer = vkCtx->swapchainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
  renderPassInfo.renderArea.extent = (VkExtent2D){WIDTH, HEIGHT}; // Use defined constants
  VkClearValue clearValues[2] = {{{{0.5f, 0.5f, 0.5f, 1.0f}}}, {{{1.0f, 0}}}};
  renderPassInfo.clearValueCount = 2;
  renderPassInfo.pClearValues = clearValues;

  vkCmdBeginRenderPass(vkCtx->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(vkCtx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx->graphicsPipeline);
  vkCmdBindDescriptorSets(vkCtx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx->pipelineLayout, 0, 1, &vkCtx->descriptorSet, 0, NULL);

  VkDeviceSize offsets[] = {0};
  if (vkCtx->triangle.exists) {
      vsdl_log("Rendering triangle with %u vertices\n", vkCtx->triangle.vertexCount);
      vkCmdBindVertexBuffers(vkCtx->commandBuffer, 0, 1, &vkCtx->triangle.buffer, offsets);
      vkCmdDraw(vkCtx->commandBuffer, vkCtx->triangle.vertexCount, 1, 0, 0);
  }
  if (vkCtx->cube.exists) {
      vsdl_log("Rendering cube with %u vertices\n", vkCtx->cube.vertexCount);
      vkCmdBindVertexBuffers(vkCtx->commandBuffer, 0, 1, &vkCtx->cube.buffer, offsets);
      vkCmdDraw(vkCtx->commandBuffer, vkCtx->cube.vertexCount, 1, 0, 0);
  }
  if (vkCtx->text.exists) {
      vsdl_log("Rendering text with %u vertices\n", vkCtx->text.vertexCount);
      vkCmdBindVertexBuffers(vkCtx->commandBuffer, 0, 1, &vkCtx->text.buffer, offsets);
      vkCmdDraw(vkCtx->commandBuffer, vkCtx->text.vertexCount, 1, 0, 0);
  }

  vkCmdEndRenderPass(vkCtx->commandBuffer);
  if (vkEndCommandBuffer(vkCtx->commandBuffer) != VK_SUCCESS) {
      vsdl_log("Failed to end command buffer\n");
      exit(1);
  }
}

void vsdl_reflect_vertex_inputs(const char* shaderCode, size_t codeSize, VkVertexInputAttributeDescription** attrDesc, uint32_t* attrCount, uint32_t* stride) {
    spvc_context context = NULL;
    spvc_parsed_ir parsed_ir = NULL;
    spvc_compiler compiler = NULL;
    spvc_resources resources = NULL;
    const spvc_reflected_resource* stage_inputs = NULL;
    size_t input_count = 0;

    if (spvc_context_create(&context) != SPVC_SUCCESS) {
        vsdl_log("Failed to create SPIRV-Cross context\n");
        exit(1);
    }

    if (spvc_context_parse_spirv(context, (const uint32_t*)shaderCode, codeSize / sizeof(uint32_t), &parsed_ir) != SPVC_SUCCESS) {
        vsdl_log("Failed to parse SPIR-V\n");
        spvc_context_destroy(context);
        exit(1);
    }

    if (spvc_context_create_compiler(context, SPVC_BACKEND_GLSL, parsed_ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler) != SPVC_SUCCESS) {
        vsdl_log("Failed to create SPIRV-Cross compiler\n");
        spvc_context_destroy(context);
        exit(1);
    }

    if (spvc_compiler_create_shader_resources(compiler, &resources) != SPVC_SUCCESS) {
        vsdl_log("Failed to create shader resources\n");
        spvc_context_destroy(context);
        exit(1);
    }

    if (spvc_resources_get_resource_list_for_type(resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &stage_inputs, &input_count) != SPVC_SUCCESS) {
        vsdl_log("Failed to get stage inputs\n");
        spvc_context_destroy(context);
        exit(1);
    }

    *attrCount = (uint32_t)input_count;
    *attrDesc = malloc(*attrCount * sizeof(VkVertexInputAttributeDescription));
    *stride = 0;

    for (uint32_t i = 0; i < *attrCount; i++) {
        uint32_t location = spvc_compiler_get_decoration(compiler, stage_inputs[i].id, SpvDecorationLocation);
        spvc_type_id type_id = stage_inputs[i].type_id;
        spvc_type type = spvc_compiler_get_type_handle(compiler, type_id);

        (*attrDesc)[i].location = location;
        (*attrDesc)[i].binding = 0;
        (*attrDesc)[i].offset = *stride;

        spvc_basetype base_type = spvc_type_get_basetype(type);
        if (base_type == SPVC_BASETYPE_FP32) {
            unsigned vector_size = spvc_type_get_vector_size(type);
            switch (vector_size) {
                case 1: (*attrDesc)[i].format = VK_FORMAT_R32_SFLOAT; break;    // TexFlag
                case 2: (*attrDesc)[i].format = VK_FORMAT_R32G32_SFLOAT; break; // TexCoord
                case 3: (*attrDesc)[i].format = VK_FORMAT_R32G32B32_SFLOAT; break; // Position or Color
                default: vsdl_log("Unsupported vector size: %u\n", vector_size); exit(1);
            }
            *stride += vector_size * sizeof(float);
        } else {
            vsdl_log("Unsupported base type: %u\n", base_type);
            exit(1);
        }

        vsdl_log("Vertex input %u: location=%u, format=%u, offset=%u\n", 
                 i, (*attrDesc)[i].location, (*attrDesc)[i].format, (*attrDesc)[i].offset);
    }

    spvc_context_destroy(context);
}