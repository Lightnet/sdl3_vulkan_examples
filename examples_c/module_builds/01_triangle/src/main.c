#include "vsdl_init.h"
#include "vsdl_mesh.h"
#include "vsdl_pipeline.h"
#include "vsdl_loop.h"
#include "vsdl_clean_up.h"

int main() {
    LOG("Starting Vulkan Triangle application");
    if (vsdl_init() || vsdl_mesh_create() || vsdl_pipeline_create()) {
        vsdl_clean_up();
        return 1;
    }
    vsdl_loop();
    vsdl_clean_up();
    LOG("Application ended successfully");
    return 0;
}