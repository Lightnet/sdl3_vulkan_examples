#ifndef VSDL_CAMERA_H
#define VSDL_CAMERA_H

#include <SDL3/SDL.h>
#include <cglm/cglm.h>
#include "vsdl_types.h" // For VulkanContext

typedef struct {
    vec3 pos;
    vec3 front;
    vec3 up;
    float yaw;
    float pitch;
} Camera;

void vsdl_reset_camera(Camera* cam);
void vsdl_update_camera(Camera* cam, SDL_Event* event, bool* mouseCaptured, SDL_Window* window, Uint32 deltaTime);
void vsdl_update_uniform_buffer(VulkanContext* vkCtx, Camera* cam, float rotationAngle); // Fixed syntax

#endif