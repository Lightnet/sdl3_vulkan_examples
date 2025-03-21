#ifndef VSDL_IMGUI_H
#define VSDL_IMGUI_H

#include "vsdl_types.h"

namespace vsdl {
    // Initialize ImGui with SDL3 and Vulkan
    bool init_imgui(VSDL_Context& ctx);

    // Process ImGui events and prepare a new frame
    void imgui_new_frame(VSDL_Context& ctx, SDL_Event& event);

    // Render ImGui draw data
    void imgui_render(VSDL_Context& ctx, VkCommandBuffer commandBuffer);

    // Shutdown ImGui
    void shutdown_imgui(VSDL_Context& ctx);
}

#endif // VSDL_IMGUI_H