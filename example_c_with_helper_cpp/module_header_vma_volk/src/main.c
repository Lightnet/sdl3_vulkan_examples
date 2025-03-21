#define VK_NO_PROTOTYPES
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "vsdl_init.h"
#include "vsdl_renderer.h"
#include "vsdl_cleanup.h"

int main(int argc, char* argv[]) {
    SDL_Log("init main\n");
    VSDL_Context ctx = {0};
    if (!vsdl_init(&ctx)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initialization failed");
        vsdl_cleanup(&ctx);
        return 1;
    }
    SDL_Log("init loop");
    if (!vsdl_render_loop(&ctx)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Render loop failed");
        vsdl_cleanup(&ctx);
        return 1;
    }
    SDL_Log("Calling cleanup");
    vsdl_cleanup(&ctx);
    return 0;
}