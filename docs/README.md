

```c
SDL_Log("init VkApplicationInfo");
VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
appInfo.pApplicationName = "Vulkan SDL3";
appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
appInfo.pEngineName = "No Engine";
appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
appInfo.apiVersion = VK_API_VERSION_1_3;

// Get the count of required Vulkan instance extensions
Uint32 extensionCount = 0;
const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
if (!extensionNames) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get Vulkan extensions: %s", SDL_GetError());
    SDL_DestroyWindow(ctx->window);
    SDL_Quit();
    return 1;
}

// Log all extensions
SDL_Log("Found %u Vulkan instance extensions:", extensionCount);
for (Uint32 i = 0; i < extensionCount; i++) {
  SDL_Log("  %u: %s", i + 1, extensionNames[i]);
}

VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledExtensionCount = extensionCount;
createInfo.ppEnabledExtensionNames = extensionNames;
SDL_Log("Init VK_LAYER_KHRONOS_validation");
const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
createInfo.enabledLayerCount = 1;
createInfo.ppEnabledLayerNames = validationLayers;

SDL_Log("init vkCreateInstance");
if (vkCreateInstance(&createInfo, NULL, &ctx->instance) != VK_SUCCESS) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan instance");
    return 0;
}
volkLoadInstance(ctx->instance);
SDL_Log("Vulkan instance created with %u extensions", 2);
```
 * SDL_Vulkan_GetInstanceExtensions
 * VK_LAYER_KHRONOS_validation