#define SDL_MAIN_HANDLED
#include "vsdl_init.h"
#include "vsdl_renderer.h"
#include "vsdl_pipeline.h"
#include "vsdl_cleanup.h"

int main(int argc, char* argv[]) {
    VSDL_Context ctx = {};
    if (!vsdl_init(ctx)) {
        vsdl_cleanup(ctx);
        return 1;
    }

    vsdl_create_pipeline(ctx);
    vsdl_render_loop(ctx);
    vsdl_cleanup(ctx);

    return 0;
}