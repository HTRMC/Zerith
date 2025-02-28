// Application.cpp
#include "Application.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>

#include <algorithm>
#include <array>

#include "ChunkStorage.hpp"
#include "Quad.hpp"
#include "QuadInstance.hpp"
#include "ShaderManager.hpp"

#ifdef _WIN32

#include <vulkan/vulkan_win32.h>

#else
#include <vulkan/vulkan_xcb.h>
#include <unistd.h>
#include <linux/limits.h>
#endif

Application::Application() : physicalDevice(VK_NULL_HANDLE)
#ifdef _WIN32
        , window(GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2)
#else
, window([]{
    xcb_connection_t* tmpConn = xcb_connect(nullptr, nullptr);
    xcb_screen_t* tmpScreen = xcb_setup_roots_iterator(xcb_get_setup(tmpConn)).data;
    int width = tmpScreen->width_in_pixels / 2;
    int height = tmpScreen->height_in_pixels / 2;
    xcb_disconnect(tmpConn);
    return std::make_pair(width, height);
}().first, []{
    xcb_connection_t* tmpConn = xcb_connect(nullptr, nullptr);
    xcb_screen_t* tmpScreen = xcb_setup_roots_iterator(xcb_get_setup(tmpConn)).data;
    int width = tmpScreen->width_in_pixels / 2;
    int height = tmpScreen->height_in_pixels / 2;
    xcb_disconnect(tmpConn);
    return std::make_pair(width, height);
}().second)
#endif
{
    appPath = getExecutablePath();
    window.setIcon(appPath + "/resources/x256.ico");
    window.setCaptureMouse(true);
}

Application::~Application() {
    cleanup();
}

void Application::run() {
    initVulkan();
    initializePlayer();
    lastFrameTime = std::chrono::steady_clock::now();
    mainLoop();
}

void Application::initVulkan() {
    createInstance();
    surface = window.createSurface(instance);
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
    ShaderManager::getInstance().init(device);
    createSwapChain();
    createImageViews();
    createRenderPass();
    debugRenderer = std::make_unique<DebugRenderer>(
            device,
            physicalDevice,
            commandPool,
            renderPass,
            swapChainExtent.width,
            swapChainExtent.height
    );
    createDescriptorPool();
    createDescriptorSetLayout();
    createSkyDescriptorSetLayout();
    createGraphicsPipeline();
    createSkyPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndirectBuffer();
    createUniformBuffers();
    createSkyColorsBuffer();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void Application::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    std::vector<const char *> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#else
            VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif
    };

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {

    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void Application::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, &createInfo, nullptr, &debugMessenger);
    }
}

void Application::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    int maxScore = 0;
    for (const auto &device: devices) {
        int score = rateDeviceSuitability(device);
        if (score > maxScore) {
            physicalDevice = device;
            maxScore = score;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
}

int Application::rateDeviceSuitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    if (!deviceFeatures.geometryShader) {
        return 0;
    }

    return score;
}

bool Application::checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName: validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties: availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void Application::mainLoop() {
    std::cout << "Application is running..." << std::endl;

    size_t currentFrame = 0;

    while (!window.shouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        window.pollEvents();

        if (window.shouldClose()) {
            break;
        }

        updateCamera();
        updateCameraRotation();
        debugRenderer->update(deltaTime);

        // Clear boxes at the start of each frame
        debugRenderer->clearBoxes();

        // Draw debug visualizations in order
        if (chunkBordersEnabled) {
            for (const auto &chunkPos: chunkPositions) {
                drawChunkDebugBox(chunkPos);
            }
        }
        drawPlayerBoundingBox();

        glm::vec3 hitPosition;
        BlockType hitBlockType;
        glm::vec3 hitNormal;

        // Update the target block (whether left mouse is pressed or not)
        hasTargetBlock = raycastBlock(hitPosition, hitBlockType, hitNormal, 100.0f);
        if (hasTargetBlock) {
            targetBlockPos = hitPosition;

            // Always draw black outline for targeted block
            AABB blockBox = getBlockAABB(hitPosition.x, hitPosition.y, hitPosition.z);
            debugRenderer->drawBox(blockBox, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0f); // Black box with no expiration
        }

        // Handle left mouse click for breaking blocks
        bool leftMouseIsPressed = window.isKeyPressed(KeyCode::MOUSE_LEFT);
        static bool leftMouseWasPressed = false;

        if (leftMouseIsPressed && !leftMouseWasPressed) {
            if (hasTargetBlock) {
                std::cout << "Breaking " << blockTypeToString(hitBlockType)
                          << " at (" << targetBlockPos.x << ", "
                          << targetBlockPos.y << ", "
                          << targetBlockPos.z << ")" << std::endl;

                // Break the block that's being looked at
                breakBlock(targetBlockPos);
            } else {
                std::cout << "Not looking at any block (within range)" << std::endl;
            }
        }
        leftMouseWasPressed = leftMouseIsPressed;

        // Check if we need to update the rendering after block modifications
        if (needsRebuild) {
            updateModifiedBlocks();
        }

        // Handle F3 + B and F3 + G key presses
        handleDebugToggles();

        try {
            drawFrame(currentFrame);
        } catch (const std::runtime_error &e) {
            std::cerr << "Error during frame rendering: " << e.what() << std::endl;
            break;
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vkDeviceWaitIdle(device);
}

void Application::cleanup() {
    vkDeviceWaitIdle(device);

    ShaderManager::getInstance().cleanup();

    debugRenderer.reset();

    vkDestroyBuffer(device, blockTypeBuffer, nullptr);
    vkFreeMemory(device, blockTypeBufferMemory, nullptr);

    vkDestroyBuffer(device, chunkIndicesBuffer, nullptr);
    vkFreeMemory(device, chunkIndicesBufferMemory, nullptr);

    vkDestroyBuffer(device, chunkPositionsBuffer, nullptr);
    vkFreeMemory(device, chunkPositionsBufferMemory, nullptr);

    vkDestroyBuffer(device, indirectBuffer, nullptr);
    vkFreeMemory(device, indirectBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkUnmapMemory(device, skyColorsBuffersMemory[i]);
        vkDestroyBuffer(device, skyColorsBuffers[i], nullptr);
        vkFreeMemory(device, skyColorsBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(device, skyDescriptorSetLayout, nullptr);
    vkDestroyPipeline(device, skyPipeline, nullptr);
    vkDestroyPipelineLayout(device, skyPipelineLayout, nullptr);

    vkDestroySampler(device, textureSampler, nullptr);
    vkDestroyImageView(device, textureImageView, nullptr);
    vkDestroyImage(device, textureImage, nullptr);
    vkFreeMemory(device, textureImageMemory, nullptr);

    vkDestroyBuffer(device, instanceBuffer, nullptr);
    vkFreeMemory(device, instanceBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    for (auto framebuffer: swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    if (enableValidationLayers) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, nullptr);
        }
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView: swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

Application::QueueFamilyIndices Application::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily: queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

void Application::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily: uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void Application::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void Application::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

Application::SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    VkExtent2D actualExtent = {
            static_cast<uint32_t>(window.getWidth()),
            static_cast<uint32_t>(window.getHeight())
    };

    actualExtent.width = std::clamp(actualExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height,
                                     capabilities.minImageExtent.height,
                                     capabilities.maxImageExtent.height);

    return actualExtent;
}

std::vector<char> Application::readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule Application::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkFormat Application::findDepthFormat() {
    return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Application::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                          VkFormatFeatureFlags features) {
    for (VkFormat format: candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

void Application::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                              VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                              VkDeviceMemory &imageMemory) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView Application::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

void Application::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Application::createRenderPass() {
    std::array<VkAttachmentDescription, 2> attachments = {};

    // Color attachment
    attachments[0].format = swapChainImageFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format = findDepthFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

std::string Application::getExecutablePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string execPath(path);
    return execPath.substr(0, execPath.find_last_of("\\/"));
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string execPath(result, (count > 0) ? count : 0);
    return execPath.substr(0, execPath.find_last_of("/"));
#endif
}

void Application::createGraphicsPipeline() {
    auto vertShaderCode = readFile(appPath + "/shaders/shader.vert.spv");
    auto fragShaderCode = readFile(appPath + "/shaders/shader.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    // Position attribute
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    // Texture coordinate attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

    // Texture ID attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R16_UINT;
    attributeDescriptions[2].offset = offsetof(Vertex, textureID);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    // rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending state
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Push constant range for model matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // Clean up shader modules
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Application::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void Application::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void Application::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Application::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void Application::drawFrame(size_t currentFrame) {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame);
    updateSkyColors(currentFrame);

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex, currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return;
    }
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Draw sky first
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            skyPipelineLayout, 0, 1, &skyDescriptorSets[currentFrame], 0, nullptr);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);  // 6 vertices for fullscreen quad

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    // Base model matrix
    glm::mat4 model = glm::mat4(1.0f);
    vkCmdPushConstants(commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

    // Draw all faces in one instanced call
    vkCmdDrawIndexedIndirect(commandBuffer, indirectBuffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));

    debugRenderer->render(commandBuffer, currentViewProj);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Application::createDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 6> bindings{};

    // Uniform buffer binding
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Texture sampler binding
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // Instance transform buffer binding
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Chunk positions buffer binding
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Chunk indices buffer binding
    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[4].descriptorCount = 1;
    bindings[4].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[5].descriptorCount = 1;
    bindings[5].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Application::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void Application::updateUniformBuffer(uint32_t currentImage) {
    UniformBufferObject ubo{};

    // Create view matrix
    ubo.view = glm::lookAt(
            cameraPos,                    // Camera position
            cameraPos + cameraFront,      // Look at point
            cameraUp                      // Up vector
    );

    // Create projection matrix
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
                                0.1f, 1000.0f);

    // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates
    // is inverted. Vulkan clip coordinates requires correcting for this.
    ubo.proj[1][1] *= -1;

    // Store the combined view-projection matrix
    currentViewProj = ubo.proj * ubo.view;

    // Add instance count
    ubo.instanceCount = instanceCount;

    void *data;
    vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

void Application::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 6> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[4].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[5].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Application::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        VkDescriptorBufferInfo instanceBufferInfo{};
        instanceBufferInfo.buffer = instanceBuffer;
        instanceBufferInfo.offset = 0;
        instanceBufferInfo.range = sizeof(uint32_t) * instanceCount;

        VkDescriptorBufferInfo chunkPositionsBufferInfo{};
        chunkPositionsBufferInfo.buffer = chunkPositionsBuffer;
        chunkPositionsBufferInfo.offset = 0;
        chunkPositionsBufferInfo.range = sizeof(ChunkStorage::ChunkPositionData) * chunkPositions.size();

        VkDescriptorBufferInfo chunkIndicesBufferInfo{};
        chunkIndicesBufferInfo.buffer = chunkIndicesBuffer;
        chunkIndicesBufferInfo.offset = 0;
        chunkIndicesBufferInfo.range = sizeof(uint32_t) * chunkIndices.size();

        VkDescriptorBufferInfo blockTypeBufferInfo{};
        blockTypeBufferInfo.buffer = blockTypeBuffer;
        blockTypeBufferInfo.offset = 0;
        blockTypeBufferInfo.range = sizeof(uint32_t) * instanceCount;

        std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &instanceBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &chunkPositionsBufferInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &chunkIndicesBufferInfo;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = descriptorSets[i];
        descriptorWrites[5].dstBinding = 5;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pBufferInfo = &blockTypeBufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags properties, VkBuffer &buffer,
                               VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

uint32_t Application::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Application::updatePlayerPhysics() {
    // Apply gravity regardless of ground state
    playerVelocity.z += gravity * deltaTime;

    // Check for ground contact directly with a ray cast
    checkGroundContact();

    // Apply friction when on ground
    if (playerOnGround) {
        playerVelocity.x *= glm::clamp(1.0f - (deltaTime * playerFriction), 0.0f, 1.0f);
        playerVelocity.y *= glm::clamp(1.0f - (deltaTime * playerFriction), 0.0f, 1.0f);
    }

    // Handle jumping (Space bar)
    if (window.isKeyPressed(KeyCode::SPACE) && playerOnGround) {
        playerVelocity.z = jumpForce;
        playerOnGround = false;
    }

    // Apply movement input to velocity
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, cameraFront.y, 0.0f));
    glm::vec3 horizontalRight = glm::normalize(glm::cross(horizontalFront, cameraUp));

    // Calculate movement direction based on input
    glm::vec3 moveDirection(0.0f);

    if (window.isKeyPressed(KeyCode::W)) {
        moveDirection += horizontalFront;
    }
    if (window.isKeyPressed(KeyCode::S)) {
        moveDirection -= horizontalFront;
    }
    if (window.isKeyPressed(KeyCode::A)) {
        moveDirection -= horizontalRight;
    }
    if (window.isKeyPressed(KeyCode::D)) {
        moveDirection += horizontalRight;
    }

    // Normalize and apply movement
    if (glm::length(moveDirection) > 0.0f) {
        moveDirection = glm::normalize(moveDirection);

        // Apply movement to velocity (with reduced influence when in air)
        float controlInfluence = playerOnGround ? 1.0f : 0.5f;

        glm::vec3 targetVelocity = moveDirection * baseMovementSpeed * controlInfluence;

        playerVelocity.x = targetVelocity.x;
        playerVelocity.y = targetVelocity.y;
    }

    // Apply velocity to position
    playerPosition += playerVelocity * deltaTime;

//    std::cout << "Player Position: (" << playerPosition.x << ", " << playerPosition.y << ", " << playerPosition.z << ")" << std::endl;

    // Resolve collisions with blocks
    resolveCollisions();

    // Update camera position based on perspective
    switch (currentPerspective) {
        case CameraPerspective::FirstPerson:
            cameraPos = playerPosition + glm::vec3(0.0f, 0.0f, 1.6f); // Add eye height offset
            break;
        case CameraPerspective::ThirdPerson: {
            // Calculate eye position
            glm::vec3 eyePosition = playerPosition + glm::vec3(0.0f, 0.0f, 1.6f);
            // Position camera behind and slightly above player's eye
            glm::vec3 offset = -cameraFront * thirdPersonDistance; // Back away from player
            cameraPos = eyePosition + offset;
            break;
        }
    }
}

void Application::updateCamera() {
    // Handle F5 key press for perspective switching
    bool f5IsPressed = window.isKeyPressed(KeyCode::F5);
    static bool f5WasPressed = false;
    if (f5IsPressed && !f5WasPressed) {
        // Cycle through perspectives
        switch (currentPerspective) {
            case CameraPerspective::FirstPerson:
                currentPerspective = CameraPerspective::ThirdPerson;
                break;
            case CameraPerspective::ThirdPerson:
                currentPerspective = CameraPerspective::FirstPerson;
                break;
        }
    }
    f5WasPressed = f5IsPressed;

    updatePlayerPhysics();
}

void Application::updateCameraRotation() {
    float deltaX = window.getMouseDeltaX() * mouseSensitivity;
    float deltaY = -window.getMouseDeltaY() * mouseSensitivity; // Inverted Y axis

    yaw -= deltaX;
    pitch += deltaY;

    // Clamp pitch to avoid flipping
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Calculate new front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.z = sin(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    window.resetMouseDeltas();
}

void Application::createVertexBuffer() {
    std::vector<Vertex> vertices = Quad::getQuadVertices();
    std::vector<uint32_t> indices = Quad::getQuadIndices();

    // Generate chunk data and get visible faces for multiple chunks
    std::vector<uint32_t> blockTypes;
    auto instances = ChunkStorage::generateMultiChunk(chunkPositions, chunkIndices, blockTypes, modifiedBlocks);

    vertexCount = static_cast<uint32_t>(indices.size());
    instanceCount = static_cast<uint32_t>(instances.size());

    VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    VkDeviceSize instanceBufferSize = sizeof(uint32_t) * instances.size();
    VkDeviceSize blockTypeBufferSize = sizeof(uint32_t) * blockTypes.size();
    VkDeviceSize chunkPositionsBufferSize = sizeof(ChunkStorage::ChunkPositionData) * chunkPositions.size();
    VkDeviceSize chunkIndicesBufferSize = sizeof(uint32_t) * chunkIndices.size();

    // Create staging buffer for vertices
    VkBuffer vertexStagingBuffer;
    VkDeviceMemory vertexStagingBufferMemory;
    createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertexStagingBuffer, vertexStagingBufferMemory);

    // Copy vertex data
    void *data;
    vkMapMemory(device, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(device, vertexStagingBufferMemory);

    // Create vertex buffer
    createBuffer(vertexBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 vertexBuffer, vertexBufferMemory);

    // Create staging buffer for indices
    VkBuffer indexStagingBuffer;
    VkDeviceMemory indexStagingBufferMemory;
    createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 indexStagingBuffer, indexStagingBufferMemory);

    // Copy index data
    vkMapMemory(device, indexStagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) indexBufferSize);
    vkUnmapMemory(device, indexStagingBufferMemory);

    // Create index buffer
    createBuffer(indexBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 indexBuffer, indexBufferMemory);

    // Create instance data buffer
    createBuffer(instanceBufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 instanceBuffer, instanceBufferMemory);

    // Create block type buffer
    VkBuffer blockTypeStagingBuffer;
    VkDeviceMemory blockTypeStagingBufferMemory;
    createBuffer(blockTypeBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 blockTypeStagingBuffer, blockTypeStagingBufferMemory);

    // Copy block type data
    void *blockTypeData;
    vkMapMemory(device, blockTypeStagingBufferMemory, 0, blockTypeBufferSize, 0, &blockTypeData);
    memcpy(blockTypeData, blockTypes.data(), (size_t) blockTypeBufferSize);
    vkUnmapMemory(device, blockTypeStagingBufferMemory);

    // Create final block type buffer
    createBuffer(blockTypeBufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 blockTypeBuffer, blockTypeBufferMemory);

    // Create staging buffer for instance data
    VkBuffer instanceStagingBuffer;
    VkDeviceMemory instanceStagingBufferMemory;
    createBuffer(instanceBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 instanceStagingBuffer, instanceStagingBufferMemory);

    // Copy instance data
    vkMapMemory(device, instanceStagingBufferMemory, 0, instanceBufferSize, 0, &data);
    memcpy(data, instances.data(), (size_t) instanceBufferSize);
    vkUnmapMemory(device, instanceStagingBufferMemory);

    // Create chunk positions buffer
    createBuffer(chunkPositionsBufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 chunkPositionsBuffer, chunkPositionsBufferMemory);

    // Create staging buffer for chunk positions
    VkBuffer chunkPositionsStagingBuffer;
    VkDeviceMemory chunkPositionsStagingBufferMemory;
    createBuffer(chunkPositionsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 chunkPositionsStagingBuffer, chunkPositionsStagingBufferMemory);

    // Copy chunk positions data
    vkMapMemory(device, chunkPositionsStagingBufferMemory, 0, chunkPositionsBufferSize, 0, &data);
    memcpy(data, chunkPositions.data(), (size_t) chunkPositionsBufferSize);
    vkUnmapMemory(device, chunkPositionsStagingBufferMemory);

    // Create chunk indices buffer
    createBuffer(chunkIndicesBufferSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 chunkIndicesBuffer, chunkIndicesBufferMemory);

    // Create staging buffer for chunk indices
    VkBuffer chunkIndicesStagingBuffer;
    VkDeviceMemory chunkIndicesStagingBufferMemory;
    createBuffer(chunkIndicesBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 chunkIndicesStagingBuffer, chunkIndicesStagingBufferMemory);

    // Copy chunk indices data
    vkMapMemory(device, chunkIndicesStagingBufferMemory, 0, chunkIndicesBufferSize, 0, &data);
    memcpy(data, chunkIndices.data(), (size_t) chunkIndicesBufferSize);
    vkUnmapMemory(device, chunkIndicesStagingBufferMemory);

    // Copy buffers
    copyBuffer(vertexStagingBuffer, vertexBuffer, vertexBufferSize);
    copyBuffer(indexStagingBuffer, indexBuffer, indexBufferSize);
    copyBuffer(instanceStagingBuffer, instanceBuffer, instanceBufferSize);
    copyBuffer(blockTypeStagingBuffer, blockTypeBuffer, blockTypeBufferSize);
    copyBuffer(chunkPositionsStagingBuffer, chunkPositionsBuffer, chunkPositionsBufferSize);
    copyBuffer(chunkIndicesStagingBuffer, chunkIndicesBuffer, chunkIndicesBufferSize);

    // Cleanup staging buffers
    vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
    vkFreeMemory(device, vertexStagingBufferMemory, nullptr);
    vkDestroyBuffer(device, indexStagingBuffer, nullptr);
    vkFreeMemory(device, indexStagingBufferMemory, nullptr);
    vkDestroyBuffer(device, instanceStagingBuffer, nullptr);
    vkFreeMemory(device, instanceStagingBufferMemory, nullptr);
    vkDestroyBuffer(device, blockTypeStagingBuffer, nullptr);
    vkFreeMemory(device, blockTypeStagingBufferMemory, nullptr);
    vkDestroyBuffer(device, chunkPositionsStagingBuffer, nullptr);
    vkFreeMemory(device, chunkPositionsStagingBufferMemory, nullptr);
    vkDestroyBuffer(device, chunkIndicesStagingBuffer, nullptr);
    vkFreeMemory(device, chunkIndicesStagingBufferMemory, nullptr);
}

void Application::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Application::createTextureImage() {
    const std::vector<std::string> textureFiles = {
            "/resources/dirt.png",                      // Index 0
            "/resources/grass_block_top.png",           // Index 1
            "/resources/grass_block_side.png",          // Index 2
            "/resources/grass_block_side_overlay.png",  // Index 3
            "/resources/stone.png",                     // Index 4
            "/resources/missing.png"                    // Index 5
    };

    int texWidth, texHeight, texChannels;
    std::string firstTexturePath = appPath + textureFiles[0];
    stbi_uc *pixels = stbi_load(firstTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkDeviceSize layerSize = texWidth * texHeight * 4;
    VkDeviceSize imageSize = layerSize * textureFiles.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);

    memcpy(data, pixels, layerSize);
    stbi_image_free(pixels);

    for (size_t i = 1; i < textureFiles.size(); i++) {
        std::string texturePath = appPath + textureFiles[i];
        pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels) {
            // If texture loading fails, use the missing texture
            std::string missingTexturePath = appPath + "/resources/missing.png";
            pixels = stbi_load(missingTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels) {
                throw std::runtime_error("failed to load missing texture!");
            }
        }

        memcpy(static_cast<char *>(data) + i * layerSize, pixels, layerSize);
        stbi_image_free(pixels);
    }

    vkUnmapMemory(device, stagingBufferMemory);

    // Create the texture array image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = textureFiles.size();
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image!");
    }

    // Allocate memory for the texture array
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate texture image memory!");
    }

    vkBindImageMemory(device, textureImage, textureImageMemory, 0);

    // Transition image layout for copying
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          textureFiles.size());

    // Copy buffer to image array
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    std::vector<VkBufferImageCopy> copyRegions;
    for (uint32_t i = 0; i < textureFiles.size(); i++) {
        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = i * layerSize;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = i;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {
                static_cast<uint32_t>(texWidth),
                static_cast<uint32_t>(texHeight),
                1
        };
        copyRegions.push_back(copyRegion);
    }

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(copyRegions.size()),
                           copyRegions.data());

    endSingleTimeCommands(commandBuffer);

    // Transition to shader reading
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          textureFiles.size());

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Application::createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;  // Changed to 2D array
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;  // Number of textures in the array
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void Application::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void Application::transitionImageLayout(VkImage image, VkFormat format,
                                        VkImageLayout oldLayout, VkImageLayout newLayout) {

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Application::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkCommandBuffer Application::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Application::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Application::transitionImageLayout(VkImage image, VkFormat format,
                                        VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount) {

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::runtime_error("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void Application::createSkyPipeline() {
    // Read shader files
    auto skyVertShaderCode = readFile(appPath + "/shaders/sky.vert.spv");
    auto skyFragShaderCode = readFile(appPath + "/shaders/sky.frag.spv");

    VkShaderModule skyVertShaderModule = createShaderModule(skyVertShaderCode);
    VkShaderModule skyFragShaderModule = createShaderModule(skyFragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = skyVertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = skyFragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // Vertex input state - we don't need any vertex input as the positions are hardcoded in the shader
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    // Input assembly - we're drawing triangles
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and stencil state
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;  // Disable depth testing for sky
    depthStencil.depthWriteEnable = VK_FALSE; // Don't write to depth buffer
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Pipeline layout (for uniforms)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &skyDescriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &skyPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create sky pipeline layout!");
    }

    // Create the graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = skyPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;  // Use the first subpass
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create sky pipeline!");
    }

    // Cleanup shader modules
    vkDestroyShaderModule(device, skyFragShaderModule, nullptr);
    vkDestroyShaderModule(device, skyVertShaderModule, nullptr);
}

void Application::createSkyDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding skyUBOBinding{};
    skyUBOBinding.binding = 0;
    skyUBOBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    skyUBOBinding.descriptorCount = 1;
    skyUBOBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    skyUBOBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &skyUBOBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &skyDescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create sky descriptor set layout!");
    }
}

void Application::createSkyColorsBuffer() {
    VkDeviceSize bufferSize = sizeof(SkyUBO);

    // Create uniform buffers for each frame in flight
    skyColorsBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    skyColorsBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    skyColorsMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                skyColorsBuffers[i],
                skyColorsBuffersMemory[i]
        );

        // Map the memory persistently
        vkMapMemory(device, skyColorsBuffersMemory[i], 0, bufferSize, 0, &skyColorsMapped[i]);

        // Initialize with default sky colors and identity view matrix
        SkyUBO defaultColors{
                glm::mat4(1.0f),                      // Identity view matrix
                glm::vec4(0.0f, 0.5f, 1.0f, 1.0f),   // Top color: light blue
                glm::vec4(0.5f, 0.7f, 1.0f, 1.0f)    // Bottom color: slightly lighter blue
        };
        memcpy(skyColorsMapped[i], &defaultColors, sizeof(SkyUBO));
    }

    // Create descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, skyDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;  // Use existing descriptor pool
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    skyDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, skyDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate sky descriptor sets!");
    }

    // Update descriptor sets
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = skyColorsBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SkyUBO);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = skyDescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
}

void Application::updateSkyColors(uint32_t currentFrame) {
    SkyUBO skyUBO{};
    skyUBO.view = glm::lookAt(
            glm::vec3(0.0f), // Camera position for sky (always at origin)
            cameraFront,      // Look direction
            cameraUp         // Up vector
    );
    skyUBO.topColor = glm::vec4(0.4706f, 0.6549f, 1.0f, 1.0f);    // Top sky color (#78a7ff)
    skyUBO.bottomColor = glm::vec4(0.7529f, 0.8471f, 1.0f, 1.0f); // Bottom sky color (#c0d8ff)

    memcpy(skyColorsMapped[currentFrame], &skyUBO, sizeof(SkyUBO));
}

void Application::createIndirectBuffer() {
    VkDrawIndexedIndirectCommand indirectCmd{};
    indirectCmd.indexCount = vertexCount;        // Number of indices
    indirectCmd.instanceCount = instanceCount;    // Total number of instances across all chunks
    indirectCmd.firstIndex = 0;                  // Starting index in index buffer
    indirectCmd.vertexOffset = 0;                // Offset added to vertex indices
    indirectCmd.firstInstance = 0;               // First instance ID

    VkDeviceSize bufferSize = sizeof(VkDrawIndexedIndirectCommand);

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    // Copy data to staging buffer
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, &indirectCmd, bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create indirect buffer
    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 indirectBuffer, indirectBufferMemory);

    // Copy data from staging buffer to indirect buffer
    copyBuffer(stagingBuffer, indirectBuffer, bufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Application::drawChunkDebugBox(const ChunkStorage::ChunkPositionData &chunkPos) {
    glm::vec3 min = chunkPos.position;
    glm::vec3 max = min + glm::vec3(ChunkStorage::CHUNK_SIZE);
    AABB chunkBox{min, max};
    debugRenderer->drawBox(chunkBox, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red wireframe
}

void Application::drawPlayerBoundingBox() {
    if (!showPlayerBoundingBox) return;

    // Calculate the box dimensions
    float halfWidth = PLAYER_WIDTH / 2.0f;
    glm::vec3 min = playerPosition - glm::vec3(halfWidth, halfWidth, 0.0f);
    glm::vec3 max = playerPosition + glm::vec3(halfWidth, halfWidth, PLAYER_HEIGHT);

    // Create AABB and draw it with white lines
    AABB playerBox{min, max};
    glm::vec4 boxColor = playerOnGround ?
                         glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) : // Green when on ground
                         glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White when in air

    debugRenderer->drawBox(playerBox, boxColor);

//    if (glm::length(playerVelocity) > 0.1f) {
//        glm::vec3 velocityEnd = playerPosition + playerVelocity * 0.5f; // Scale for visibility
//        debugRenderer->drawLine(playerPosition, velocityEnd, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
//    }
}

void Application::handleDebugToggles() {
    bool currentF3State = window.isKeyPressed(KeyCode::F3);
    bool currentBState = window.isKeyPressed(KeyCode::B);
    bool currentGState = window.isKeyPressed(KeyCode::G);

    // Handle F3 + B for player bounding box
    if (currentF3State && currentBState && (!f3KeyPressed || !bKeyPressed)) {
        showPlayerBoundingBox = !showPlayerBoundingBox;
        f3KeyPressed = true;
        bKeyPressed = true;
    } else if (!currentF3State) {
        f3KeyPressed = false;
    } else if (!currentBState) {
        bKeyPressed = false;
    }

    // Handle F3 + G for chunk borders
    if (currentF3State && currentGState && (!f3KeyPressed || !gKeyPressed)) {
        chunkBordersEnabled = !chunkBordersEnabled;
        debugRenderer->clearBoxes();
        if (chunkBordersEnabled) {
            for (const auto &chunkPos: chunkPositions) {
                drawChunkDebugBox(chunkPos);
            }
        }
        f3KeyPressed = true;
        gKeyPressed = true;
    } else if (!currentF3State) {
        f3KeyPressed = false;
    } else if (!currentGState) {
        gKeyPressed = false;
    }
}

BlockType Application::getBlockTypeAt(const glm::vec3 &worldPos) {
    // Convert world position to block coordinates
    int blockX = static_cast<int>(floor(worldPos.x));
    int blockY = static_cast<int>(floor(worldPos.y));
    int blockZ = static_cast<int>(floor(worldPos.z));

    // Check if this block has been modified
    std::string blockKey = getBlockKey(blockX, blockY, blockZ);
    auto it = modifiedBlocks.find(blockKey);
    if (it != modifiedBlocks.end()) {
        return it->second;
    }

    // If not modified, proceed with original logic
    // Calculate chunk coordinates
    const int chunkSize = ChunkStorage::CHUNK_SIZE;

    // Fixed calculation for negative coordinates
    // Integer division in C++ rounds towards zero, but we need floor division
    int chunkX = (blockX < 0) ? ((blockX + 1 - chunkSize) / chunkSize) : (blockX / chunkSize);
    int chunkY = (blockY < 0) ? ((blockY + 1 - chunkSize) / chunkSize) : (blockY / chunkSize);

    // Calculate local coordinates within chunk
    int localX = blockX - chunkX * chunkSize;
    int localY = blockY - chunkY * chunkSize;
    int localZ = blockZ;

    // Verification step for debugging
    if (localX < 0) localX += chunkSize;
    if (localY < 0) localY += chunkSize;

    // Ensure local coordinates are always in range [0, chunkSize)
    localX = (localX + chunkSize) % chunkSize;
    localY = (localY + chunkSize) % chunkSize;

    // Ensure coordinates are valid
    if (localX < 0 || localX >= chunkSize ||
        localY < 0 || localY >= chunkSize ||
        localZ < 0 || localZ >= chunkSize) {
        return BlockType::AIR;
    }

    // Find the chunk
    for (size_t i = 0; i < chunkPositions.size(); i++) {
        const auto &chunk = chunkPositions[i];
        glm::vec3 chunkWorldPos = chunk.position;
        int posChunkX = static_cast<int>(chunkWorldPos.x) / chunkSize;
        int posChunkY = static_cast<int>(chunkWorldPos.y) / chunkSize;

        if (posChunkX == chunkX && posChunkY == chunkY) {
            // Simplified block type lookup
            // In a real implementation, you would access your actual block data
            // This is a simplified version to demonstrate the concept
            if (localZ <= 0) {
                return BlockType::BEDROCK;
            }

            // Simplified height-based terrain
            ChunkStorage::BlockGrid blocks = ChunkStorage::generateTestChunk(chunkX, chunkY);
            return blocks[localX][localY][localZ];
        }
    }

    // If no chunk found, return air
    return BlockType::AIR;
}

bool Application::isPositionSolid(const glm::vec3 &worldPos) {
    BlockType type = getBlockTypeAt(worldPos);
    return type != BlockType::AIR && type != BlockType::WATER;
}

AABB Application::getBlockAABB(int x, int y, int z) {
    // Convert block coordinates to world coordinates
    glm::vec3 min = glm::vec3(x, y, z);
    glm::vec3 max = min + glm::vec3(1.0f);

    return AABB{min, max};
}

void Application::resolveCollisions() {
    // Create player AABB
    float halfWidth = PLAYER_WIDTH / 2.0f;
    glm::vec3 playerMin = playerPosition - glm::vec3(halfWidth, halfWidth, 0.0f);
    glm::vec3 playerMax = playerPosition + glm::vec3(halfWidth, halfWidth, PLAYER_HEIGHT);
    AABB playerBox{playerMin, playerMax};

    // Reset ground check
    playerOnGround = false;

    // Check blocks in player's vicinity
    int minX = static_cast<int>(floor(playerMin.x - 1));
    int maxX = static_cast<int>(floor(playerMax.x + 1));
    int minY = static_cast<int>(floor(playerMin.y - 1));
    int maxY = static_cast<int>(floor(playerMax.y + 1));
    int minZ = static_cast<int>(floor(playerMin.z - 1));
    int maxZ = static_cast<int>(floor(playerMax.z + 1));

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                if (isPositionSolid(glm::vec3(x, y, z))) {
                    AABB blockBox = getBlockAABB(x, y, z);

                    if (playerBox.intersects_aabb(blockBox)) {
                        // Calculate overlap on each axis
                        float overlapX = std::min(playerBox.max.x - blockBox.min.x, blockBox.max.x - playerBox.min.x);
                        float overlapY = std::min(playerBox.max.y - blockBox.min.y, blockBox.max.y - playerBox.min.y);
                        float overlapZ = std::min(playerBox.max.z - blockBox.min.z, blockBox.max.z - playerBox.min.z);

                        // Resolve with smallest overlap (most efficient resolution)
                        if (overlapX < overlapY && overlapX < overlapZ) {
                            // X-axis resolution
                            if (playerPosition.x > blockBox.centre().x) {
                                playerPosition.x += overlapX;
                            } else {
                                playerPosition.x -= overlapX;
                            }
                            // Stop X velocity on collision
                            playerVelocity.x = 0;
                        } else if (overlapY < overlapX && overlapY < overlapZ) {
                            // Y-axis resolution
                            if (playerPosition.y > blockBox.centre().y) {
                                playerPosition.y += overlapY;
                            } else {
                                playerPosition.y -= overlapY;
                            }
                            // Stop Y velocity on collision
                            playerVelocity.y = 0;
                        } else {
                            // Z-axis resolution
                            if (playerPosition.z > blockBox.centre().z) {
                                playerPosition.z += overlapZ;
                                // Stop Z velocity on collision
                                playerVelocity.z = 0;
                            } else {
                                playerPosition.z -= overlapZ;
                                // Check if player is on ground
                                if (playerVelocity.z < 0) {
                                    playerOnGround = true;
                                }
                                // Stop Z velocity on collision
                                playerVelocity.z = 0;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Application::initializePlayer() {
    // Start the player at a reasonable height above the terrain
    playerPosition = glm::vec3(0.0f, 0.0f, 20.0f); // Start above ground level
    playerVelocity = glm::vec3(0.0f);
    playerOnGround = false;

    // Initialize camera
    cameraPos = playerPosition + glm::vec3(0.0f, 0.0f, 1.6f);
    cameraFront = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
    cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
}

void Application::checkGroundContact() {
    // Reset ground state
    playerOnGround = false;

    // Define a small distance to check below the player
    const float groundCheckDistance = 0.05f;

    // Get the player's feet position
    float halfWidth = PLAYER_WIDTH / 2.0f;

    // Check multiple points along the player's base for more reliable detection
    glm::vec3 checkPoints[5] = {
            // Center point
            glm::vec3(playerPosition.x, playerPosition.y, playerPosition.z),
            // Four corners
            glm::vec3(playerPosition.x - halfWidth + 0.1f, playerPosition.y - halfWidth + 0.1f, playerPosition.z),
            glm::vec3(playerPosition.x + halfWidth - 0.1f, playerPosition.y - halfWidth + 0.1f, playerPosition.z),
            glm::vec3(playerPosition.x - halfWidth + 0.1f, playerPosition.y + halfWidth - 0.1f, playerPosition.z),
            glm::vec3(playerPosition.x + halfWidth - 0.1f, playerPosition.y + halfWidth - 0.1f, playerPosition.z)
    };

    // Check each point
    for (int i = 0; i < 5; i++) {
        glm::vec3 checkPos = checkPoints[i];

        // Check if there's a block right below the player
        glm::vec3 pointBelow = checkPos - glm::vec3(0.0f, 0.0f, groundCheckDistance);

        // Convert to block coordinates
        int blockX = static_cast<int>(floor(pointBelow.x));
        int blockY = static_cast<int>(floor(pointBelow.y));
        int blockZ = static_cast<int>(floor(pointBelow.z));

        // Check if the block below is solid
        if (isPositionSolid(glm::vec3(blockX, blockY, blockZ))) {
            playerOnGround = true;
            return;
        }
    }
}

bool
Application::raycastBlock(glm::vec3 &outHitPosition, BlockType &outBlockType, glm::vec3 &outNormal, float maxDistance) {
    // Ray origin (camera position)
    glm::vec3 origin = cameraPos;

    // Ray direction (camera front vector, normalized)
    glm::vec3 direction = glm::normalize(cameraFront);

    // Voxel traversal using DDA (Digital Differential Analyzer) algorithm
    // Initialize starting voxel (block coordinates)
    int blockX = static_cast<int>(floor(origin.x));
    int blockY = static_cast<int>(floor(origin.y));
    int blockZ = static_cast<int>(floor(origin.z));

    // Calculate ray length when it crosses cell boundaries
    // deltaT is the distance the ray needs to travel to move one unit in each dimension
    float deltaDistX = (direction.x != 0.0f) ? std::abs(1.0f / direction.x) : FLT_MAX;
    float deltaDistY = (direction.y != 0.0f) ? std::abs(1.0f / direction.y) : FLT_MAX;
    float deltaDistZ = (direction.z != 0.0f) ? std::abs(1.0f / direction.z) : FLT_MAX;

    // Current length of ray from origin to block boundary
    float tMaxX, tMaxY, tMaxZ;

    // Direction to step in (+1 or -1)
    int stepX, stepY, stepZ;

    // Initialize step direction and initial tMax values
    if (direction.x < 0.0f) {
        stepX = -1;
        tMaxX = (origin.x - blockX) * deltaDistX;
    } else {
        stepX = 1;
        tMaxX = (blockX + 1.0f - origin.x) * deltaDistX;
    }

    if (direction.y < 0.0f) {
        stepY = -1;
        tMaxY = (origin.y - blockY) * deltaDistY;
    } else {
        stepY = 1;
        tMaxY = (blockY + 1.0f - origin.y) * deltaDistY;
    }

    if (direction.z < 0.0f) {
        stepZ = -1;
        tMaxZ = (origin.z - blockZ) * deltaDistZ;
    } else {
        stepZ = 1;
        tMaxZ = (blockZ + 1.0f - origin.z) * deltaDistZ;
    }

    // Perform ray marching with early termination
    float currentDistance = 0.0f;
    int faceIndex = -1; // 0 = X, 1 = Y, 2 = Z

    // Maximum number of steps (safety measure)
    const int MAX_STEPS = 100;
    int steps = 0;

    while (currentDistance < maxDistance && steps < MAX_STEPS) {
        // Step to the next voxel
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            currentDistance = tMaxX;
            blockX += stepX;
            tMaxX += deltaDistX;
            faceIndex = 0;
        } else if (tMaxY < tMaxZ) {
            currentDistance = tMaxY;
            blockY += stepY;
            tMaxY += deltaDistY;
            faceIndex = 1;
        } else {
            currentDistance = tMaxZ;
            blockZ += stepZ;
            tMaxZ += deltaDistZ;
            faceIndex = 2;
        }

        // Check if the current block is solid
        BlockType blockType = getBlockTypeAt({blockX, blockY, blockZ});
        if (blockType != BlockType::AIR) {
            // Set hit information
            outHitPosition = glm::vec3(blockX, blockY, blockZ);
            outBlockType = blockType;

            // Calculate hit normal based on the face we entered from
            outNormal = glm::vec3(0.0f);
            if (faceIndex == 0) { // X face
                outNormal.x = -stepX;
            } else if (faceIndex == 1) { // Y face
                outNormal.y = -stepY;
            } else { // Z face
                outNormal.z = -stepZ;
            }

            return true;
        }

        steps++;
    }

    // No hit found within max distance
    return false;
}

std::string Application::blockTypeToString(BlockType type) {
    switch (type) {
        case BlockType::AIR:
            return "Air";
        case BlockType::STONE:
            return "Stone";
        case BlockType::GRASS_BLOCK:
            return "Grass Block";
        case BlockType::DIRT:
            return "Dirt";
        case BlockType::BEDROCK:
            return "Bedrock";
        case BlockType::WATER:
            return "Water";
        default:
            return "Unknown Block";
    }
}

std::string Application::getBlockKey(int x, int y, int z) const {
    return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
}

void Application::breakBlock(const glm::vec3& position) {
    // Convert world position to block coordinates
    int blockX = static_cast<int>(floor(position.x));
    int blockY = static_cast<int>(floor(position.y));
    int blockZ = static_cast<int>(floor(position.z));

    // Don't allow breaking blocks at or below z=0 (bedrock layer)
    if (blockZ <= 0) {
        std::cout << "Cannot break bedrock!" << std::endl;
        return;
    }

    // Create a key for this block position
    std::string blockKey = getBlockKey(blockX, blockY, blockZ);

    // Store the modified block state in our map (set to AIR for breaking)
    modifiedBlocks[blockKey] = BlockType::AIR;

    std::cout << "Block broken at (" << blockX << ", " << blockY << ", " << blockZ << ")" << std::endl;

    // Flag that we need to update our vertex buffers
    needsRebuild = true;
}

// Method to rebuild the vertex buffers when blocks have changed
void Application::updateModifiedBlocks() {
    if (!needsRebuild) {
        return;
    }

    // Clean up old buffers
    vkDeviceWaitIdle(device);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, instanceBuffer, nullptr);
    vkFreeMemory(device, instanceBufferMemory, nullptr);
    vkDestroyBuffer(device, blockTypeBuffer, nullptr);
    vkFreeMemory(device, blockTypeBufferMemory, nullptr);

    // Recreate vertex buffer with updated block data
    createVertexBuffer();

    // Also recreate indirect buffer with new instance count
    vkDestroyBuffer(device, indirectBuffer, nullptr);
    vkFreeMemory(device, indirectBufferMemory, nullptr);
    createIndirectBuffer();

    needsRebuild = false;
}