#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Xinput.h>

#include "rendering/Vertex.hpp"
#include "rendering/ModelLoader.hpp"
#include "rendering/TextureLoader.hpp"
#include "world/ChunkManager.hpp"

// Window dimensions
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// Validation layers
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Device extensions
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// Queue family indices structure
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// Swap chain support details structure
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Main Vulkan application class
class VulkanApp {
public:
    void run();

private:
    // Window handle
    HWND window = NULL;
    HINSTANCE hInstance = NULL;

    // Vulkan instance and debug
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // Physical device and logical device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    // Queues
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    // Surface
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // Swap chain
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // Render pass and pipeline
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkPipeline opaquePipeline = VK_NULL_HANDLE;
    VkPipeline cutoutPipeline = VK_NULL_HANDLE;
    VkPipeline translucentPipeline = VK_NULL_HANDLE;

    // Create separate pipelines for different render layers
    void createRenderLayerPipelines();

    // Create command buffers that render all three layers in order
    void createMultiLayerCommandBuffers();

    // Framebuffers
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Command pool and buffers
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    // Synchronization objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    // Window resize tracking
    bool framebufferResized = false;

    // Maximum frames in flight
    const int MAX_FRAMES_IN_FLIGHT = 2;

    // Window functions
    void initWindow();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Initialization functions
    void initVulkan();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    // Swap chain recreation
    void cleanupSwapChain();
    void recreateSwapChain();

    // Helper functions
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);

    // Main loop and cleanup
    void mainLoop();
    void drawFrame();
    void cleanup();

    // Vertex and index buffers
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    // Uniform buffer for transformation matrix
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    // Uniform buffer object for transformations
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    // Model data
    ModelData currentModel;
    ModelLoader modelLoader;
    TextureLoader textureLoader;

    // New methods for model loading
    bool loadBlockBenchModel(const std::string& filename);
    void createVertexBufferFromModel();
    void createIndexBufferFromModel();
    uint32_t loadModelTextures();

    // Previous methods
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void updateUniformBuffer();
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    // Depth buffer
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    // Add these function declarations
    void createDepthResources();
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    bool hasStencilComponent(VkFormat format);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                    VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    // Camera properties
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 32.0f);
    glm::vec3 cameraFront = glm::vec3(-0.5f, -0.5f, -0.5f); // Normalized in constructor
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
    float cameraSpeed = 2.0f;

    // Input handling
    struct KeyState {
        bool w = false;
        bool a = false;
        bool s = false;
        bool d = false;
        bool space = false;
        bool shift = false;
    } keys;

    float deltaTime = 0.0f; // Time between frames
    float lastFrameTime = 0.0f; // Time of last frame

    // Process key input (add to private methods)
    void processInput();
    void updateCamera();

    static VulkanApp* appInstance;

    // Mouse handling
    struct MouseState {
        bool firstMouse = true;
        float lastX = WIDTH / 2.0f;
        float lastY = HEIGHT / 2.0f;
        float yaw = -90.0f;   // Yaw is initialized to -90 to make initial direction face along negative z-axis
        float pitch = 0.0f;
        bool captured = false;
    } mouseState;

    // Function to process mouse input
    void processMouseInput(int x, int y);
    void updateCameraDirection();
    void toggleMouseCapture();

    // Gamepad state
    struct GamepadState {
        bool connected = false;
        float leftStickX = 0.0f;
        float leftStickY = 0.0f;
        float rightStickX = 0.0f;
        float rightStickY = 0.0f;
        float leftTrigger = 0.0f;
        float rightTrigger = 0.0f;
        bool rightStickButton = false;
        bool bottomButton = false;
    } gamepadState;

    // Gamepad methods
    void updateGamepadInput();
    float processGamepadStickValue(SHORT value, float deadzone);

    // Chunk management
    ChunkManager chunkManager;

    // Chunk loading configuration
    int chunkLoadRadius = 2;         // Default chunk load radius
    float chunkUpdateInterval = 0.5f; // Time between chunk updates in seconds
    float lastChunkUpdateTime = 0.0f; // Last time chunks were updated

    // Set up the chunk loading system
    void setupChunkSystem();

    // Update loaded chunks based on camera position
    void updateLoadedChunks();
};

// Debug messenger callback function
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

// Debug messenger creation helper function
VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

// Debug messenger destruction helper function
void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);