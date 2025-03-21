// vsdl_renderer.cpp
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_log.h>
#include "vsdl_renderer.h"
#include "vsdl_types.h"
#include "vsdl_pipeline.h"

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

    for (const auto& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    VkExtent2D extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
    extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
    return extent;
}

static void updateUniformBuffer(VSDL_Context& ctx, float posX, float posY, float rotZ) {
    UniformBufferObject ubo = {};
    float cosZ = cosf(rotZ), sinZ = sinf(rotZ);

    ubo.transform[0] = cosZ;   // m00
    ubo.transform[1] = sinZ;   // m01
    ubo.transform[2] = 0.0f;   // m02
    ubo.transform[3] = 0.0f;   // m03
    ubo.transform[4] = -sinZ;  // m10
    ubo.transform[5] = cosZ;   // m11
    ubo.transform[6] = 0.0f;   // m12
    ubo.transform[7] = 0.0f;   // m13
    ubo.transform[8] = 0.0f;   // m20
    ubo.transform[9] = 0.0f;   // m21
    ubo.transform[10] = 1.0f;  // m22
    ubo.transform[11] = 0.0f;  // m23
    ubo.transform[12] = posX;  // m30 (translation X)
    ubo.transform[13] = posY;  // m31 (translation Y)
    ubo.transform[14] = 0.0f;  // m32
    ubo.transform[15] = 1.0f;  // m33

    void* data;
    vmaMapMemory(ctx.allocator, ctx.uniformBufferAllocation, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vmaUnmapMemory(ctx.allocator, ctx.uniformBufferAllocation);
}

static bool recreateSwapchain(VSDL_Context& ctx) {
    vkDeviceWaitIdle(ctx.device);

    for (auto& fb : ctx.framebuffers) {
        vkDestroyFramebuffer(ctx.device, fb, nullptr);
    }
    ctx.framebuffers.clear();
    for (auto& view : ctx.swapchainImageViews) {
        vkDestroyImageView(ctx.device, view, nullptr);
    }
    ctx.swapchainImageViews.clear();
    vkDestroySwapchainKHR(ctx.device, ctx.swapchain, nullptr);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &capabilities);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(ctx.physicalDevice, ctx.surface);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(ctx.physicalDevice, ctx.surface);
    VkExtent2D extent = chooseSwapExtent(capabilities, ctx.window);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = ctx.surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(ctx.device, &swapchainInfo, nullptr, &ctx.swapchain) != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to recreate swapchain");
        return false;
    }
    SDL_Log("Swapchain recreated");

    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, nullptr);
    ctx.swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, ctx.swapchainImages.data());
    ctx.swapchainImageFormat = surfaceFormat.format;
    ctx.swapchainExtent = extent;

    ctx.swapchainImageViews.resize(swapchainImageCount);
    for (size_t i = 0; i < swapchainImageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = ctx.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = ctx.swapchainImageFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &ctx.swapchainImageViews[i]) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to recreate image view %zu", i);
            return false;
        }
    }
    SDL_Log("Swapchain image views recreated (count: %u)", swapchainImageCount);

    ctx.framebuffers.resize(swapchainImageCount);
    for (size_t i = 0; i < swapchainImageCount; i++) {
        VkImageView attachments[] = {ctx.swapchainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = ctx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = ctx.swapchainExtent.width;
        framebufferInfo.height = ctx.swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx.device, &framebufferInfo, nullptr, &ctx.framebuffers[i]) != VK_SUCCESS) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to recreate framebuffer %zu", i);
            return false;
        }
    }
    SDL_Log("Framebuffers recreated (count: %zu)", ctx.framebuffers.size());

    return true;
}

bool vsdl_render_loop(VSDL_Context& ctx) {
  SDL_Log("Starting render loop");

  if (!SDL_Vulkan_CreateSurface(ctx.window, ctx.instance, nullptr, &ctx.surface)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan surface: %s", SDL_GetError());
      return false;
  }

  VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = ctx.graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(ctx.device, &poolInfo, nullptr, &ctx.commandPool) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create command pool");
      vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
      return false;
  }
  SDL_Log("Command pool created");

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice, ctx.surface, &capabilities);
  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(ctx.physicalDevice, ctx.surface);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(ctx.physicalDevice, ctx.surface);
  VkExtent2D extent = chooseSwapExtent(capabilities, ctx.window);

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
      imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainInfo.surface = ctx.surface;
  swapchainInfo.minImageCount = imageCount;
  swapchainInfo.imageFormat = surfaceFormat.format;
  swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapchainInfo.imageExtent = extent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.preTransform = capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = presentMode;
  swapchainInfo.clipped = VK_TRUE;
  swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(ctx.device, &swapchainInfo, nullptr, &ctx.swapchain) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swapchain");
      vkDestroyCommandPool(ctx.device, ctx.commandPool, nullptr);
      vkDestroySurfaceKHR(ctx.instance, ctx.surface, nullptr);
      return false;
  }
  SDL_Log("Swapchain created");

  uint32_t swapchainImageCount;
  vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, nullptr);
  ctx.swapchainImages.resize(swapchainImageCount);
  vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &swapchainImageCount, ctx.swapchainImages.data());
  ctx.swapchainImageFormat = surfaceFormat.format;
  ctx.swapchainExtent = extent;

  ctx.swapchainImageViews.resize(swapchainImageCount);
  for (size_t i = 0; i < swapchainImageCount; i++) {
      VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      viewInfo.image = ctx.swapchainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = ctx.swapchainImageFormat;
      viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;

      if (vkCreateImageView(ctx.device, &viewInfo, nullptr, &ctx.swapchainImageViews[i]) != VK_SUCCESS) {
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create image view %zu", i);
          return false;
      }
  }
  SDL_Log("Swapchain image views created (count: %u)", swapchainImageCount);

  if (!create_pipeline(ctx)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create pipeline");
      return false;
  }

  if (!recreateSwapchain(ctx)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create initial swapchain and framebuffers");
      return false;
  }

  VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  if (vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(ctx.device, &semaphoreInfo, nullptr, &ctx.renderFinishedSemaphore) != VK_SUCCESS ||
      vkCreateFence(ctx.device, &fenceInfo, nullptr, &ctx.frameFence) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create synchronization objects");
      return false;
  }
  SDL_Log("Synchronization objects created");

  VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = ctx.commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  if (vkAllocateCommandBuffers(ctx.device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate command buffer");
      return false;
  }

  SDL_ShowWindow(ctx.window);

  float posX = 0.0f, posY = 0.0f;
  float rotZ = 0.0f;
  const float moveSpeed = 0.01f;
  const float rotSpeed = 0.02f;
  bool swapchainNeedsRecreate = false;

  bool running = true;
  SDL_Event event;
  SDL_Log("Entering render loop");
  while (running) {
      SDL_Log("Polling events");
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_EVENT_QUIT) {
              SDL_Log("Quit event received");
              running = false;
          }
          if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_MAXIMIZED || event.type == SDL_EVENT_WINDOW_RESTORED) {
              SDL_Log("Window resize event: %dx%d", event.window.data1, event.window.data2);
              swapchainNeedsRecreate = true;
          }
          if (event.type == SDL_EVENT_KEY_DOWN) {
              switch (event.key.key) {
                  case SDLK_W: posY += moveSpeed; break;
                  case SDLK_S: posY -= moveSpeed; break;
                  case SDLK_A: posX -= moveSpeed; break;
                  case SDLK_D: posX += moveSpeed; break;
                  case SDLK_5: rotZ += rotSpeed; break;
                  case SDLK_6: rotZ -= rotSpeed; break;
              }
          }
      }

      // Skip rendering if window is minimized
      int width, height;
      SDL_GetWindowSize(ctx.window, &width, &height);
      if (width == 0 || height == 0) {
          SDL_Log("Window minimized, skipping frame");
          SDL_Delay(100); // Avoid busy-waiting
          continue;
      }

      if (swapchainNeedsRecreate) {
          SDL_Log("Recreating swapchain");
          if (!recreateSwapchain(ctx)) {
              SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Swapchain recreation failed");
              running = false;
              continue;
          }
          swapchainNeedsRecreate = false;
      }

      updateUniformBuffer(ctx, posX, posY, rotZ);

      SDL_Log("Acquiring next image");
      vkWaitForFences(ctx.device, 1, &ctx.frameFence, VK_TRUE, UINT64_MAX);
      vkResetFences(ctx.device, 1, &ctx.frameFence);

      uint32_t imageIndex;
      VkResult result = vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, ctx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          SDL_Log("Swapchain out of date or suboptimal");
          swapchainNeedsRecreate = true;
          continue;
      }
      if (result != VK_SUCCESS) {
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire swapchain image: %d", result);
          running = false;
          continue;
      }

      VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(commandBuffer, &beginInfo);

      VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
      renderPassInfo.renderPass = ctx.renderPass;
      renderPassInfo.framebuffer = ctx.framebuffers[imageIndex];
      renderPassInfo.renderArea.offset = {0, 0};
      renderPassInfo.renderArea.extent = ctx.swapchainExtent;
      VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
      renderPassInfo.clearValueCount = 1;
      renderPassInfo.pClearValues = &clearColor;

      vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.graphicsPipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.pipelineLayout, 0, 1, &ctx.descriptorSet, 0, nullptr);

      VkViewport viewport = {0.0f, 0.0f, (float)ctx.swapchainExtent.width, (float)ctx.swapchainExtent.height, 0.0f, 1.0f};
      VkRect2D scissor = {{0, 0}, ctx.swapchainExtent};
      vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
      vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

      VkBuffer vertexBuffers[] = {ctx.vertexBuffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
      vkCmdDraw(commandBuffer, 3, 1, 0, 0);
      vkCmdEndRenderPass(commandBuffer);
      vkEndCommandBuffer(commandBuffer);

      VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
      VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = &ctx.imageAvailableSemaphore;
      submitInfo.pWaitDstStageMask = waitStages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = &ctx.renderFinishedSemaphore;

      if (vkQueueSubmit(ctx.graphicsQueue, 1, &submitInfo, ctx.frameFence) != VK_SUCCESS) {
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to submit draw command buffer");
          running = false;
          continue;
      }

      VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = &ctx.renderFinishedSemaphore;
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = &ctx.swapchain;
      presentInfo.pImageIndices = &imageIndex;

      result = vkQueuePresentKHR(ctx.graphicsQueue, &presentInfo);
      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
          SDL_Log("Swapchain out of date after present");
          swapchainNeedsRecreate = true;
      } else if (result != VK_SUCCESS) {
          SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to present queue: %d", result);
          running = false;
      }
      SDL_Log("Frame rendered");
  }

  SDL_Log("Render loop ended");
  vkQueueWaitIdle(ctx.graphicsQueue);

  vkFreeCommandBuffers(ctx.device, ctx.commandPool, 1, &commandBuffer);
  if (ctx.frameFence) {
      SDL_Log("Destroying frame fence");
      vkDestroyFence(ctx.device, ctx.frameFence, nullptr);
      ctx.frameFence = VK_NULL_HANDLE;
  }
  if (ctx.renderFinishedSemaphore) {
      SDL_Log("Destroying render finished semaphore");
      vkDestroySemaphore(ctx.device, ctx.renderFinishedSemaphore, nullptr);
      ctx.renderFinishedSemaphore = VK_NULL_HANDLE;
  }
  if (ctx.imageAvailableSemaphore) {
      SDL_Log("Destroying image available semaphore");
      vkDestroySemaphore(ctx.device, ctx.imageAvailableSemaphore, nullptr);
      ctx.imageAvailableSemaphore = VK_NULL_HANDLE;
  }

  return true;
}