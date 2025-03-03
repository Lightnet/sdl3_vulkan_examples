#ifndef VSDL_RENDER_H
#define VSDL_RENDER_H

#include <vulkan/vulkan.h>
#include "vsdl_types.h" // Use vsdl_types.h for VulkanContext instead of vsdl_vulkan_init.h

void vsdl_create_pipeline(VulkanContext* vkCtx);
void vsdl_record_command_buffer(VulkanContext* vkCtx, uint32_t imageIndex);
void vsdl_reflect_vertex_inputs(const char* shaderCode, size_t codeSize, VkVertexInputAttributeDescription** attrDesc, uint32_t* attrCount, uint32_t* stride);

#endif