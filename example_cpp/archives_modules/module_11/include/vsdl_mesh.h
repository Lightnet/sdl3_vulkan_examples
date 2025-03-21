// vsdl_mesh.h
#ifndef VSDL_MESH_H
#define VSDL_MESH_H

#include "vsdl_types.h"

bool create_triangle_buffer(VSDL_Context& ctx);
bool destroy_mesh(VSDL_Context& ctx); // Added
bool create_plane_buffer(VSDL_Context& ctx);
bool destroy_mesh(VSDL_Context& ctx);
bool create_uniform_buffer(VSDL_Context& ctx);
#endif