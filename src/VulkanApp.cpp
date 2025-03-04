#include "VulkanApp.hpp"

#include <Windows.h>
#include <windowsx.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <cstring>

VulkanApp *VulkanApp::appInstance = nullptr;

// Debug callback function
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

// Create debug utils messenger extension function
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Destroy debug utils messenger extension function
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// Window procedure callback
LRESULT CALLBACK VulkanApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (appInstance) {
                switch (wParam) {
                    case 'W': appInstance->keys.w = true;
                        break;
                    case 'A': appInstance->keys.a = true;
                        break;
                    case 'S': appInstance->keys.s = true;
                        break;
                    case 'D': appInstance->keys.d = true;
                        break;
                    case VK_SPACE: appInstance->keys.space = true;
                        break;
                    case VK_SHIFT: appInstance->keys.shift = true;
                        break;
                    case VK_ESCAPE:
                        if (appInstance->mouseState.captured) {
                            appInstance->toggleMouseCapture();
                        }
                        break;
                }
            }
            return 0;
        case WM_KEYUP:
            if (appInstance) {
                switch (wParam) {
                    case 'W': appInstance->keys.w = false;
                        break;
                    case 'A': appInstance->keys.a = false;
                        break;
                    case 'S': appInstance->keys.s = false;
                        break;
                    case 'D': appInstance->keys.d = false;
                        break;
                    case VK_SPACE: appInstance->keys.space = false;
                        break;
                    case VK_SHIFT: appInstance->keys.shift = false;
                        break;
                }
            }
            return 0;
        case WM_RBUTTONDOWN:
            if (appInstance) {
                // Only toggle if not already captured
                if (!appInstance->mouseState.captured) {
                    appInstance->toggleMouseCapture();
                }
            }
            return 0;
        case WM_RBUTTONUP:
            // Optional: handle right button release if needed
            return 0;
        case WM_MOUSEMOVE:
            if (appInstance && appInstance->mouseState.captured) {
                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);
                appInstance->processMouseInput(xPos, yPos);
            }
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// Main run function
void VulkanApp::run() {
    // Set the static application instance for window procedure
    appInstance = this;

    // Normalize camera front vector
    cameraFront = glm::normalize(cameraFront);

    // Initialize the mouse state
    mouseState.yaw = -90.0f; // Start looking along negative Z axis
    mouseState.pitch = 0.0f;

    // Initialize time
    lastFrameTime = static_cast<float>(GetTickCount64()) / 1000.0f;

    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

// Initialize the window
void VulkanApp::initWindow() {
    hInstance = GetModuleHandle(nullptr);

    // Register the window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "ZerithVulkanWindow";

    RegisterClassEx(&wc);

    // Create the window
    RECT rect = {0, 0, WIDTH, HEIGHT};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    window = CreateWindowEx(
        0,
        "ZerithVulkanWindow",
        "Zerith Vulkan Cube",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!window) {
        throw std::runtime_error("Failed to create window!");
    }

    ShowWindow(window, SW_SHOW);
}

// Initialize Vulkan
void VulkanApp::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandPool();

    // Initialize the texture loader
    textureLoader.init(device, physicalDevice, commandPool, graphicsQueue);

    // Load the BlockBench model
    if (!loadBlockBenchModel("resources/models/oak_stairs.json")) {
        std::cout << "Failed to load BlockBench model, falling back to hardcoded cube" << std::endl;
        createVertexBuffer();
        createIndexBuffer();
    } else {
        // Load textures for the model
        uint32_t textureId = loadModelTextures();
        if (textureId != textureLoader.getDefaultTextureId()) {
            currentModel.textureId = textureId;
            std::cout << "Loaded texture for model: " << textureId << std::endl;
        }

        // Create vertex and index buffers from the loaded model
        createVertexBufferFromModel();
        createIndexBufferFromModel();
    }

    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

// Create Vulkan instance
void VulkanApp::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    // Application info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Zerith Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    // Create info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Extensions
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Validation layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

// Setup debug messenger
void VulkanApp::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger!");
    }
}

// Create window surface
void VulkanApp::createSurface() {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = window;
    createInfo.hinstance = hInstance;

    if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

// Pick physical device
void VulkanApp::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device: devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

// Create logical device
void VulkanApp::createLogicalDevice() {
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

    // Device features
    VkPhysicalDeviceFeatures deviceFeatures{};

    // Device create info
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Device extensions
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // Validation layers
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create the logical device
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // Get queue handles
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

// Create swap chain
void VulkanApp::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // Decide how many images in the swap chain
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // Create swap chain info
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Handle different queue families
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

    // Create swap chain
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // Get swap chain images
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

// Create image views
void VulkanApp::createImageViews() {
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
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

// Create render pass
void VulkanApp::createRenderPass() {
    // Color attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Attachment references
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Subpass dependencies
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Render pass
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

// Create graphics pipeline
void VulkanApp::createGraphicsPipeline() {
    // First, create the descriptor set layout
    createDescriptorSetLayout();

    try {
        // Load pre-compiled SPIR-V shader binaries from files
        std::vector<char> vertShaderCode = readFile("shaders/vert.spv");
        std::vector<char> fragShaderCode = readFile("shaders/frag.spv");

        // Print shader file sizes for debugging
        std::cout << "Loaded vertex shader: " << vertShaderCode.size() << " bytes" << std::endl;
        std::cout << "Loaded fragment shader: " << fragShaderCode.size() << " bytes" << std::endl;

        // Create shader modules
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        // Shader stage creation
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

        // Vertex input state - UPDATED for our Vertex struct
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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

        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // UPDATED for GLM
        rasterizer.depthBiasEnable = VK_FALSE;

        // Multisample state
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth stencil state - NEEDED for 3D rendering
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // Color blend state
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Add our descriptor set layout

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout!");
        }

        // Graphics pipeline creation
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil; // Add depth stencil state
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        // Cleanup shader modules
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    } catch (const std::exception &e) {
        std::cerr << "Error in createGraphicsPipeline: " << e.what() << std::endl;
        throw;
    }
}

// Create framebuffers
void VulkanApp::createFramebuffers() {
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
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

// Create command pool
void VulkanApp::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

// Create command buffers
void VulkanApp::createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        // Clear values for color and depth
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.149f, 0.549f, 0.894f, 1.0f}}; // Blue background
        clearValues[1].depthStencil = {1.0f, 0}; // Far depth value

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Bind vertex and index buffers
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        // Bind descriptor sets
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSet, 0, nullptr);

        // Determine the number of indices to draw
        uint32_t indexCount = currentModel.loaded ? static_cast<uint32_t>(currentModel.indices.size()) : 36;

        // Draw the model (with the appropriate number of indices, 1 instance, no offsets)
        vkCmdDrawIndexed(commandBuffers[i], indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }
}

// Create synchronization objects
void VulkanApp::createSyncObjects() {
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
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

// Main loop
void VulkanApp::mainLoop() {
    MSG msg;
    bool running = true;

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (running) {
            processInput();
            drawFrame();
        }
    }

    vkDeviceWaitIdle(device);
}

// Draw frame
void VulkanApp::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                            VK_NULL_HANDLE, &imageIndex);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // Update uniform buffer with latest transformations
    updateUniformBuffer();

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Cleanup
void VulkanApp::cleanup() {
    // Wait for the device to be idle before cleaning up
    vkDeviceWaitIdle(device);

    textureLoader.cleanup();

    // Clean up sync objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    // Clean up command pool
    vkDestroyCommandPool(device, commandPool, nullptr);

    // Clean up framebuffers
    for (auto framebuffer: swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    // Clean up pipeline
    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    // Clean up image views
    for (auto imageView: swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    // Clean up depth resources
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    // Clean up swap chain
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    // Clean up descriptor pool and layout
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // Clean up uniform buffer
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);

    // Clean up vertex and index buffers
    vkDestroyBuffer(device, indexBuffer, nullptr);
    vkFreeMemory(device, indexBufferMemory, nullptr);
    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    // Clean up device
    vkDestroyDevice(device, nullptr);

    // Clean up debug messenger if enabled
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    // Clean up surface and instance
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    // Destroy window
    DestroyWindow(window);
}

// Check validation layer support
bool VulkanApp::checkValidationLayerSupport() {
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

// Get required extensions
std::vector<const char *> VulkanApp::getRequiredExtensions() {
    std::vector<const char *> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

// Populate debug messenger create info
void VulkanApp::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// Check if device is suitable
bool VulkanApp::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

// Find queue families
QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice device) {
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

// Check device extension support
bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension: availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

// Query swap chain support
SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice device) {
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

// Choose swap surface format
VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// Choose swap present mode
VkPresentModeKHR VulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// Choose swap extent
VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        RECT rect;
        GetClientRect(window, &rect);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(rect.right - rect.left),
            static_cast<uint32_t>(rect.bottom - rect.top)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

// Create shader module
VkShaderModule VulkanApp::createShaderModule(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

// Read file helper
std::vector<char> VulkanApp::readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

// Create vertex buffer implementation
void VulkanApp::createVertexBuffer() {
    // Define the vertices of our cube
    std::vector<Vertex> vertices = {
        // Front face (z = 0.5)
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}}, // 0: bottom-left, red
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}, // 1: bottom-right, green
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}, // 2: top-right, blue
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}, // 3: top-left, white

        // Back face (z = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // 4: bottom-left, yellow
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 5: bottom-right, magenta
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 6: top-right, cyan
        {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}} // 7: top-left, gray
    };

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // Create a staging buffer (host visible)
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    // Copy vertex data to the staging buffer
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the actual vertex buffer (device local)
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Create index buffer implementation
void VulkanApp::createIndexBuffer() {
    // Define indices for the cube
    std::vector<uint16_t> indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Back face
        5, 4, 7, 7, 6, 5,
        // Left face
        4, 0, 3, 3, 7, 4,
        // Bottom face
        4, 5, 1, 1, 0, 4,
        // Top face
        3, 2, 6, 6, 7, 3
    };

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    // Copy index data to the staging buffer
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the actual index buffer
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &indexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer!");
    }

    vkGetBufferMemoryRequirements(device, indexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &indexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate index buffer memory!");
    }

    vkBindBufferMemory(device, indexBuffer, indexBufferMemory, 0);

    // Copy from staging buffer to index buffer
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Find memory type utility function
uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

// Buffer copy utility function
void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

// Begin single time commands
VkCommandBuffer VulkanApp::beginSingleTimeCommands() {
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

// End single time commands
void VulkanApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// Create descriptor set layout
void VulkanApp::createDescriptorSetLayout() {
    // UBO binding for transformation matrices
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    // Sampler binding for textures
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding,
        samplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

// Create uniform buffers
void VulkanApp::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create uniform buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate uniform buffer memory!");
    }

    vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
}

// Create descriptor pool
void VulkanApp::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

// Create descriptor sets
void VulkanApp::createDescriptorSets() {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }

    // Buffer info for UBO
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    // Image info for texture
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = textureLoader.getTextureImageView(currentModel.textureId);
    imageInfo.sampler = textureLoader.getTextureSampler();

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

    // UBO descriptor
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    // Texture descriptor
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

// Update uniform buffer
void VulkanApp::updateUniformBuffer() {
    UniformBufferObject ubo{};

    // Static model rotation
    ubo.model = glm::mat4(1.0f);

    // Position the camera
    ubo.view = glm::lookAt(
        cameraPos, // Camera position
        cameraPos + cameraFront, // Look at position
        cameraUp // Up vector
    );

    // Perspective projection
    float aspectRatio = swapChainExtent.width / (float) swapChainExtent.height;
    ubo.proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);

    // Vulkan has Y flipped compared to OpenGL, so we need to flip it back
    ubo.proj[1][1] *= -1;

    // Copy the UBO data to the uniform buffer
    void *data;
    vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBufferMemory);
}

// Create depth resources
void VulkanApp::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Find supported depth format
VkFormat VulkanApp::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

// Find supported format from candidates
VkFormat VulkanApp::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
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

    throw std::runtime_error("Failed to find supported format!");
}

// Check if format has stencil component
bool VulkanApp::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Create image
void VulkanApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                            VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkImage &image, VkDeviceMemory &imageMemory) {
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
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

// Create image view
VkImageView VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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
        throw std::runtime_error("Failed to create image view!");
    }

    return imageView;
}

// Process camera movement based on key states
void VulkanApp::processInput() {
    // Calculate delta time for smooth movement
    float currentTime = static_cast<float>(GetTickCount64()) / 1000.0f;
    deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    // Update gamepad state
    updateGamepadInput();

    // Update camera position based on input
    updateCamera();
}

// Update camera position based on user input
void VulkanApp::updateCamera() {
    float velocity = cameraSpeed * deltaTime;

    // Create a horizontal front vector by zeroing out the vertical component
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, cameraFront.y, 0.0f));

    // If the camera is looking straight up or down, horizontalFront might be zero
    // In that case, use the previous non-zero horizontalFront or a default direction
    if (glm::length(horizontalFront) < 0.1f) {
        // Use the right vector crossed with the world up to get a forward direction
        glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 right = glm::cross(cameraFront, worldUp);
        horizontalFront = glm::normalize(glm::cross(worldUp, right));
    }

    // Calculate right vector for horizontal movement
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, glm::vec3(0.0f, 0.0f, 1.0f)));

    // === Keyboard movement ===
    // Forward/backward movement (along horizontal plane)
    if (keys.w)
        cameraPos += horizontalFront * velocity;
    if (keys.s)
        cameraPos -= horizontalFront * velocity;

    // Left/right movement (along horizontal plane)
    if (keys.a)
        cameraPos -= right * velocity;
    if (keys.d)
        cameraPos += right * velocity;

    // Vertical movement only (along world up axis)
    if (keys.space)
        cameraPos += glm::vec3(0.0f, 0.0f, 1.0f) * velocity;
    if (keys.shift)
        cameraPos -= glm::vec3(0.0f, 0.0f, 1.0f) * velocity;

    // === Gamepad movement ===
    if (gamepadState.connected) {
        // Left stick - movement
        if (fabs(gamepadState.leftStickY) > 0.0f)
            cameraPos += horizontalFront * velocity * gamepadState.leftStickY;
        if (fabs(gamepadState.leftStickX) > 0.0f)
            cameraPos += right * velocity * gamepadState.leftStickX;

        // Bottom button (A/X) for flying up
        if (gamepadState.bottomButton)
            cameraPos += glm::vec3(0.0f, 0.0f, 1.0f) * velocity;

        // Right stick button for flying down
        if (gamepadState.rightStickButton)
            cameraPos -= glm::vec3(0.0f, 0.0f, 1.0f) * velocity;

        // Right stick - camera rotation
        if (fabs(gamepadState.rightStickX) > 0.0f || fabs(gamepadState.rightStickY) > 0.0f) {
            // Adjust camera rotation based on right stick
            float rotationSpeed = 0.1f; // Significantly reduce the rotation speed
            mouseState.yaw -= gamepadState.rightStickX * rotationSpeed; // Horizontal camera rotation
            mouseState.pitch += gamepadState.rightStickY * rotationSpeed; // Vertical camera rotation

            // Clamp pitch to avoid flipping
            if (mouseState.pitch > 89.0f)
                mouseState.pitch = 89.0f;
            if (mouseState.pitch < -89.0f)
                mouseState.pitch = -89.0f;

            // Update camera direction based on new angles
            updateCameraDirection();
        }
    }
}

// Toggle mouse capture state
void VulkanApp::toggleMouseCapture() {
    mouseState.captured = !mouseState.captured;

    if (mouseState.captured) {
        // Hide cursor and capture it
        ShowCursor(FALSE);

        // Get current cursor position
        POINT point;
        GetCursorPos(&point);
        ScreenToClient(window, &point);

        // Set initial position
        mouseState.lastX = static_cast<float>(point.x);
        mouseState.lastY = static_cast<float>(point.y);
        mouseState.firstMouse = true;

        // Clip cursor to window
        RECT rect;
        GetClientRect(window, &rect);
        ClientToScreen(window, reinterpret_cast<POINT *>(&rect.left));
        ClientToScreen(window, reinterpret_cast<POINT *>(&rect.right));
        ClipCursor(&rect);
    } else {
        // Show cursor and release it
        ShowCursor(TRUE);
        ClipCursor(NULL);
    }
}

// Process mouse movement for camera rotation
void VulkanApp::processMouseInput(int x, int y) {
    if (mouseState.firstMouse) {
        mouseState.lastX = static_cast<float>(x);
        mouseState.lastY = static_cast<float>(y);
        mouseState.firstMouse = false;
        return;
    }

    // Calculate mouse movement deltas
    float xOffset = static_cast<float>(x) - mouseState.lastX;
    float yOffset = mouseState.lastY - static_cast<float>(y); // Reversed: y-coordinates go from bottom to top

    mouseState.lastX = static_cast<float>(x);
    mouseState.lastY = static_cast<float>(y);

    // Adjust sensitivity
    float sensitivity = 0.1f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    // Update camera angles
    mouseState.yaw -= xOffset;
    mouseState.pitch += yOffset;

    // Clamp pitch to avoid flipping
    if (mouseState.pitch > 89.0f)
        mouseState.pitch = 89.0f;
    if (mouseState.pitch < -89.0f)
        mouseState.pitch = -89.0f;

    // Update camera direction based on new angles
    updateCameraDirection();

    // If cursor reaches edge of window, reset to center
    RECT rect;
    GetClientRect(window, &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // Check if cursor is near the edge
    bool resetCursor = false;
    if (x <= 1 || x >= windowWidth - 1 || y <= 1 || y >= windowHeight - 1) {
        resetCursor = true;
    }

    if (resetCursor) {
        POINT center = {windowWidth / 2, windowHeight / 2};
        ClientToScreen(window, &center);
        SetCursorPos(center.x, center.y);
        mouseState.lastX = static_cast<float>(windowWidth / 2);
        mouseState.lastY = static_cast<float>(windowHeight / 2);
    }
}

// Update camera direction based on yaw and pitch
void VulkanApp::updateCameraDirection() {
    // Calculate new direction vector (modified for Z-up system)
    glm::vec3 direction;
    // X component uses sin(yaw) instead of cos(yaw)
    direction.x = sin(glm::radians(mouseState.yaw)) * cos(glm::radians(mouseState.pitch));
    // Y component uses cos(yaw) with negative sign
    direction.y = -cos(glm::radians(mouseState.yaw)) * cos(glm::radians(mouseState.pitch));
    // Z component takes the sin(pitch)
    direction.z = sin(glm::radians(mouseState.pitch));

    // Update camera front vector with normalized direction
    cameraFront = glm::normalize(direction);
}

// Update the gamepad input state
void VulkanApp::updateGamepadInput() {
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Get the state of the gamepad (only checking controller 0 for simplicity)
    DWORD result = XInputGetState(0, &state);

    if (result == ERROR_SUCCESS) {
        // Controller is connected
        gamepadState.connected = true;

        // Process left stick values (movement)
        gamepadState.leftStickX = processGamepadStickValue(state.Gamepad.sThumbLX, 0.15f);
        gamepadState.leftStickY = processGamepadStickValue(state.Gamepad.sThumbLY, 0.15f);

        // Process right stick values (camera rotation)
        gamepadState.rightStickX = processGamepadStickValue(state.Gamepad.sThumbRX, 0.20f);
        gamepadState.rightStickY = processGamepadStickValue(state.Gamepad.sThumbRY, 0.20f);

        // Process triggers (vertical movement - up/down)
        gamepadState.leftTrigger = static_cast<float>(state.Gamepad.bLeftTrigger) / 255.0f;
        gamepadState.rightTrigger = static_cast<float>(state.Gamepad.bRightTrigger) / 255.0f;

        // Check button states for vertical movement
        // Right Thumb (RS) button for flying down
        gamepadState.rightStickButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

        // Bottom button (A on Xbox, X on PlayStation) for flying up
        gamepadState.bottomButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    } else {
        // Controller is not connected
        gamepadState.connected = false;
        gamepadState.leftStickX = 0.0f;
        gamepadState.leftStickY = 0.0f;
        gamepadState.rightStickX = 0.0f;
        gamepadState.rightStickY = 0.0f;
        gamepadState.leftTrigger = 0.0f;
        gamepadState.rightTrigger = 0.0f;
        gamepadState.rightStickButton = false;
        gamepadState.bottomButton = false;
    }
}

// Process gamepad stick values with deadzone
float VulkanApp::processGamepadStickValue(SHORT value, float deadzone) {
    // Convert to float in range [-1, 1]
    float result = static_cast<float>(value) / 32768.0f;

    // Apply deadzone
    if (fabs(result) < deadzone) {
        return 0.0f;
    }

    // Adjust for deadzone and normalize to still get full range
    return (result - (result > 0 ? deadzone : -deadzone)) / (1.0f - deadzone);
}

// Load a BlockBench model from a JSON file
bool VulkanApp::loadBlockBenchModel(const std::string &filename) {
    std::cout << "Loading BlockBench model: " << filename << std::endl;

    // Try to load the model
    auto modelOpt = modelLoader.loadModel(filename);

    if (!modelOpt.has_value()) {
        std::cerr << "Failed to load model from " << filename << std::endl;
        return false;
    }

    // Store the loaded model
    currentModel = modelOpt.value();
    std::cout << "Model loaded successfully. Vertices: " << currentModel.vertices.size()
            << ", Indices: " << currentModel.indices.size() << std::endl;

    // Log all resolved texture mappings
    std::cout << "Model texture mappings:" << std::endl;
    for (const auto& [key, path] : currentModel.textureMap) {
        std::cout << "  " << key << " -> " << path << std::endl;
    }

    return true;
}

// Create vertex buffer from the loaded model
void VulkanApp::createVertexBufferFromModel() {
    if (!currentModel.loaded) {
        throw std::runtime_error("Attempted to create vertex buffer without a loaded model");
    }

    VkDeviceSize bufferSize = sizeof(currentModel.vertices[0]) * currentModel.vertices.size();

    // Create a staging buffer (host visible)
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    // Copy vertex data to the staging buffer
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, currentModel.vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the actual vertex buffer (device local)
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

// Create index buffer from the loaded model
void VulkanApp::createIndexBufferFromModel() {
    if (!currentModel.loaded) {
        throw std::runtime_error("Attempted to create index buffer without a loaded model");
    }

    VkDeviceSize bufferSize = sizeof(currentModel.indices[0]) * currentModel.indices.size();

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    // Copy index data to the staging buffer
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, currentModel.indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    // Create the actual index buffer
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &indexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer!");
    }

    vkGetBufferMemoryRequirements(device, indexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &indexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate index buffer memory!");
    }

    vkBindBufferMemory(device, indexBuffer, indexBufferMemory, 0);

    // Copy from staging buffer to index buffer
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

uint32_t VulkanApp::loadModelTextures() {
    uint32_t textureId = textureLoader.getDefaultTextureId();

    // If we have a texture map in the model
    if (!currentModel.textureMap.empty()) {
        // Try loading textures in order of preference:
        // 1. 'all' texture (used for all faces)
        // 2. 'side' texture (commonly used)
        // 3. Any of bottom/top/north/south/east/west
        // 4. First texture in the map as fallback

        const std::array<std::string, 8> preferredTextures = {
            "all", "side", "bottom", "top", "north", "south", "east", "west"
        };

        for (const auto& texName : preferredTextures) {
            auto it = currentModel.textureMap.find(texName);
            if (it != currentModel.textureMap.end()) {
                textureId = textureLoader.loadTexture(it->second);
                std::cout << "Using texture for model: " << it->first << " -> " << it->second << std::endl;
                return textureId;
            }
        }

        // Fall back to the first texture in the map
        auto it = currentModel.textureMap.begin();
        textureId = textureLoader.loadTexture(it->second);
        std::cout << "Using fallback texture for model: " << it->first << " -> " << it->second << std::endl;
    } else {
        std::cout << "Model has no textures, using default texture" << std::endl;
    }

    return textureId;
}