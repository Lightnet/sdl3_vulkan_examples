#include "vsdl_camera.h"
#include "vsdl_log.h"
#include "vsdl_vulkan_init.h" // For allocator
#include <stdio.h>

void vsdl_reset_camera(Camera* cam) {
    cam->pos[0] = 0.0f; cam->pos[1] = 0.0f; cam->pos[2] = 3.0f;
    cam->front[0] = 0.0f; cam->front[1] = 0.0f; cam->front[2] = -1.0f;
    cam->up[0] = 0.0f; cam->up[1] = 1.0f; cam->up[2] = 0.0f;
    cam->yaw = -90.0f;
    cam->pitch = 0.0f;
    vsdl_log("Camera reset: Pos [0, 0, 3], Yaw -90, Pitch 0\n");
}

void vsdl_update_camera(Camera* cam, SDL_Event* event, bool* mouseCaptured, SDL_Window* window, Uint32 deltaTime) {
    const float speed = 2.5f;
    float moveSpeed = speed * (deltaTime / 1000.0f);

    if (event->type == SDL_EVENT_KEY_DOWN) {
        switch (event->key.key) {
            case SDLK_W: {
                vec3 move; glm_vec3_scale(cam->front, moveSpeed, move); glm_vec3_add(cam->pos, move, cam->pos);
                vsdl_log("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            case SDLK_S: {
                vec3 move; glm_vec3_scale(cam->front, -moveSpeed, move); glm_vec3_add(cam->pos, move, cam->pos);
                vsdl_log("Camera Pos: [%.2f, %.2f, %.2f], Front: [%.2f, %.2f, %.2f]\n", cam->pos[0], cam->pos[1], cam->pos[2], cam->front[0], cam->front[1], cam->front[2]);
                break;
            }
            // ... (rest of update_camera)
        }
    }
    // ... (rest of update_camera)
}

void vsdl_update_uniform_buffer(VulkanContext* vkCtx, Camera* cam, float rotationAngle) {
    typedef struct { mat4 model; mat4 view; mat4 proj; } UBO;
    UBO ubo;
    glm_mat4_identity(ubo.model);
    glm_rotate_y(ubo.model, glm_rad(rotationAngle), ubo.model);
    glm_lookat(cam->pos, (vec3){cam->pos[0] + cam->front[0], cam->pos[1] + cam->front[1], cam->pos[2] + cam->front[2]}, cam->up, ubo.view);
    glm_perspective(glm_rad(45.0f), 800.0f / 600.0f, 0.1f, 100.0f, ubo.proj);
    ubo.proj[1][1] *= -1;

    void* data;
    vmaMapMemory(allocator, vkCtx->uniformAllocation, &data);
    memcpy(data, &ubo, sizeof(UBO));
    vmaUnmapMemory(allocator, vkCtx->uniformAllocation);
}