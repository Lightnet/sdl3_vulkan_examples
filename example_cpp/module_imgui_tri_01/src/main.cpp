#include "vsdl_init.h"
#include "vsdl_pipeline.h"
#include "vsdl_renderer.h"
#include "vsdl_cleanup.h"
#include <SDL3/SDL_log.h>

int main(int argc, char* argv[]) {
    VSDL_Context ctx = {};

    // Initialize Vulkan and SDL
    if (!vsdl_init(ctx)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initialization failed");
        vsdl_cleanup(ctx);
        return -1;
    }

    // Create pipeline and ImGui setup
    try {
        vsdl_create_pipeline(ctx);
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pipeline creation failed: %s", e.what());
        vsdl_cleanup(ctx);
        return -1;
    }

    // Run the render loop
    try {
        vsdl_render_loop(ctx);
    } catch (const std::exception& e) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Render loop failed: %s", e.what());
        vsdl_cleanup(ctx);
        return -1;
    }

    // Cleanup
    vsdl_cleanup(ctx);
    return 0;
}