#ifndef VSDL_MESH_H
#define VSDL_MESH_H

#include "vsdl_types.h"
#include <ft2build.h>
#include FT_FREETYPE_H

extern FT_Library ft_library;

uint32_t vsdl_find_memory_type(VulkanContext* vkCtx, uint32_t typeFilter, VkMemoryPropertyFlags properties);
void vsdl_create_triangle(VulkanContext* vkCtx, RenderObject* triangle);
void vsdl_destroy_triangle(VulkanContext* vkCtx, RenderObject* triangle);
void vsdl_create_cube(VulkanContext* vkCtx, RenderObject* cube); // Ensure this matches
void vsdl_destroy_cube(VulkanContext* vkCtx, RenderObject* cube);
void vsdl_create_text(VulkanContext* vkCtx, RenderObject* text);
void vsdl_destroy_text(VulkanContext* vkCtx, RenderObject* text);

#endif