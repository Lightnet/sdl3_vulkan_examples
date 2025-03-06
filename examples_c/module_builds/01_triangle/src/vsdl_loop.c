#include "vsdl_loop.h"

void vsdl_loop(void) {
    LOG("Entering main loop");
    SDL_Event event;
    int running = 1;

    VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    vkCreateSemaphore(g_context.device, &semaphoreInfo, NULL, &g_context.imageAvailableSemaphore);
    vkCreateSemaphore(g_context.device, &semaphoreInfo, NULL, &g_context.renderFinishedSemaphore);
    vkCreateFence(g_context.device, &fenceInfo, NULL, &g_context.inFlightFence);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = 0;
        }

        vkWaitForFences(g_context.device, 1, &g_context.inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(g_context.device, 1, &g_context.inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(g_context.device, g_context.swapchain, UINT64_MAX, g_context.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(g_context.commandBuffer, 0);
        VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        vkBeginCommandBuffer(g_context.commandBuffer, &beginInfo);

        VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = g_context.renderPass,
            .framebuffer = g_context.framebuffers[imageIndex],
            .renderArea.extent = {800, 600},
            .clearValueCount = 1,
            .pClearValues = &(VkClearValue){{{0.0f, 0.0f, 0.0f, 1.0f}}}
        };
        vkCmdBeginRenderPass(g_context.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(g_context.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_context.graphicsPipeline);
        vkCmdDraw(g_context.commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(g_context.commandBuffer);
        vkEndCommandBuffer(g_context.commandBuffer);

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &g_context.imageAvailableSemaphore,
            .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
            .commandBufferCount = 1,
            .pCommandBuffers = &g_context.commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &g_context.renderFinishedSemaphore
        };
        vkQueueSubmit(g_context.graphicsQueue, 1, &submitInfo, g_context.inFlightFence);

        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &g_context.renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &g_context.swapchain,
            .pImageIndices = &imageIndex
        };
        vkQueuePresentKHR(g_context.graphicsQueue, &presentInfo);
    }
    vkQueueWaitIdle(g_context.graphicsQueue);
    LOG("Exiting main loop");
}