#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

// Global Vulkan objects
VkInstance instance = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;

// Function to create Vulkan instance
VkInstance SDL_VKCreateInstance(void) {
    // Get the count of required Vulkan instance extensions
    Uint32 extensionCount = 0;
    const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensionNames) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to get Vulkan extensions: %s", SDL_GetError());
        return VK_NULL_HANDLE;
    }

    // Log all extensions
    SDL_Log("Found %u Vulkan instance extensions:", extensionCount);
    for (Uint32 i = 0; i < extensionCount; i++) {
        SDL_Log("  %u: %s", i + 1, extensionNames[i]);
    }

    // Setup Vulkan application info
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Vulkan SDL3";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Setup Vulkan instance create info
    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = extensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames;

    // Create Vulkan instance
    VkInstance newInstance;
    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &newInstance);
    if (result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan instance: %d", result);
        return VK_NULL_HANDLE;
    }

    SDL_Log("Vulkan instance created successfully with %u extensions", extensionCount);
    return newInstance;
}

// Function to create Vulkan device
VkDevice SDL_VKCreateDevice(VkInstance inst) {
    if (inst == VK_NULL_HANDLE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid Vulkan instance provided");
        return VK_NULL_HANDLE;
    }

    // Get physical device count
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(inst, &deviceCount, NULL);
    SDL_Log("vkEnumeratePhysicalDevices found: %u devices", deviceCount);
    if (deviceCount == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No Vulkan-capable devices found");
        return VK_NULL_HANDLE;
    }

    // Enumerate physical devices
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(inst, &deviceCount, devices);

    // Select first device and log its properties
    VkPhysicalDevice physicalDevice = devices[0];  // Use first device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    SDL_Log("Selected physical device[0]: %s (Type: %d, Vendor: %u, DeviceID: %u)",
            deviceProperties.deviceName,
            deviceProperties.deviceType,
            deviceProperties.vendorID,
            deviceProperties.deviceID);
    free(devices);

    // Find queue family with graphics support
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t graphicsQueueFamily = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamily = i;
            break;
        }
    }
    free(queueFamilies);

    if (graphicsQueueFamily == UINT32_MAX) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No graphics queue family found");
        return VK_NULL_HANDLE;
    }

    // Setup queue create info
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Setup device create info (no extensions for now)
    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.ppEnabledExtensionNames = NULL;

    // Create device
    VkDevice newDevice;
    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &newDevice);
    if (result != VK_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan device: %d", result);
        return VK_NULL_HANDLE;
    }

    SDL_Log("Vulkan device created successfully with graphics queue family %u", graphicsQueueFamily);
    return newDevice;
}

int main(int argc, char *argv[])
{
    SDL_Log("Starting Vulkan SDL3 application");

    // Initialize SDL with video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL could not initialize: %s", SDL_GetError());
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("SDLVulk Test",
                                        800, 600,
                                        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create Vulkan instance and store it in global variable
    instance = SDL_VKCreateInstance();
    if (instance == VK_NULL_HANDLE) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create Vulkan device and store it in global variable
    device = SDL_VKCreateDevice(instance);
    if (device == VK_NULL_HANDLE) {
        vkDestroyInstance(instance, NULL);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Cleanup
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}