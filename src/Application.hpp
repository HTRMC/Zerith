// Application.hpp
#pragma once
#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <optional>
#include <algorithm>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ChunkStorage.hpp"
#include "DebugRenderer.hpp"
#include "Subchunk.hpp"
#include "Window.hpp"

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    std::chrono::steady_clock::time_point lastFrameTime;
    float deltaTime = 0.0f;
    float baseMovementSpeed = 4.317f * 10;
    float walkMovementSpeed = 4.317f;

    Window window;
    VkSurfaceKHR surface;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::string getExecutablePath();
    std::string appPath;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::chrono::high_resolution_clock::time_point startTime;
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
    glm::vec3 cameraFront = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
    float cameraSpeed = 0.01f;
    // float yaw = -135.0f;   // Initial yaw for looking at center (-135 degrees)
    // float pitch = -35.264f; // Initial pitch for looking at center (arctan(1/√2))
    float yaw = 0.0f;
    float pitch = 0.0f;
    float mouseSensitivity = 0.1f;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    size_t vertexCount;
    uint32_t instanceCount;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
    VkPipeline skyPipeline;
    VkPipelineLayout skyPipelineLayout;
    VkBuffer skyColorsBuffer;
    VkDeviceMemory skyColorsMemory;
    VkDescriptorSetLayout skyDescriptorSetLayout;
    std::vector<VkDescriptorSet> skyDescriptorSets;
    std::vector<VkBuffer> skyColorsBuffers;
    std::vector<VkDeviceMemory> skyColorsBuffersMemory;
    std::vector<void*> skyColorsMapped;
    VkBuffer indirectBuffer;
    VkDeviceMemory indirectBufferMemory;
    VkBuffer chunkPositionsBuffer;
    VkDeviceMemory chunkPositionsBufferMemory;
    std::vector<ChunkStorage::ChunkPositionData> chunkPositions;
    VkBuffer chunkIndicesBuffer;
    VkDeviceMemory chunkIndicesBufferMemory;
    std::vector<uint32_t> chunkIndices;
    VkBuffer blockTypeBuffer;
    VkDeviceMemory blockTypeBufferMemory;
    std::unique_ptr<DebugRenderer> debugRenderer;
    glm::mat4 currentViewProj;
    bool chunkBordersEnabled = false;
    bool f3KeyPressed = false;
    bool bKeyPressed = false;
    bool gKeyPressed = false;

    enum class CameraPerspective {
        FirstPerson,
        ThirdPerson,
        // TODO: Implement second person camera
    };

    CameraPerspective currentPerspective = CameraPerspective::FirstPerson;
    float thirdPersonDistance = 5.0f; // Distance of camera from player in third person
    glm::vec3 playerPosition = glm::vec3(0.0f, 0.0f, 10.0f); // Player's position

    bool showPlayerBoundingBox = false;
    const float PLAYER_HEIGHT = 1.8f;
    const float PLAYER_WIDTH = 0.6f;

    // Physics properties
    glm::vec3 playerVelocity = glm::vec3(0.0f);
    const float gravity = -20.0f;
    bool playerOnGround = false;
    const float jumpForce = 8.0f;
    const float playerFriction = 10.0f;

    // Collision detection helpers
    BlockType getBlockTypeAt(const glm::vec3& worldPos);
    bool isPositionSolid(const glm::vec3& worldPos);
    AABB getBlockAABB(int x, int y, int z);
    void resolveCollisions();

    // Movement method with physics
    void updatePlayerPhysics();

    struct VkDrawIndexedIndirectCommand {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t firstInstance;
    };

    struct SkyUBO {
        glm::mat4 view;
        glm::vec4 topColor;
        glm::vec4 bottomColor;
    };

    struct UniformBufferObject {
        glm::mat4 view;
        glm::mat4 proj;
        uint32_t instanceCount;
        glm::vec3 padding;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    // Validation layer support
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    void initVulkan();
    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    int rateDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createSwapChain();
    void createImageViews();
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createRenderPass();
    void createGraphicsPipeline();
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    VkFormat findDepthFormat();

    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &imageMemory);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

    void createDepthResources();

    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void drawFrame(size_t currentFrame);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void updateUniformBuffer(uint32_t currentImage);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties, VkBuffer& buffer,
                     VkDeviceMemory& bufferMemory);
    void updateCamera();
    void updateCameraRotation();

    void createVertexBuffer();

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void transitionImageLayout(VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkCommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t layerCount);

    void createSkyPipeline();
    void createSkyDescriptorSetLayout();
    void createSkyColorsBuffer();
    void updateSkyColors(uint32_t currentFrame);

    void createIndirectBuffer();

    void drawChunkDebugBox(const ChunkStorage::ChunkPositionData &chunkPos);

    void drawPlayerBoundingBox();

    void handleDebugToggles();

    bool checkValidationLayerSupport();
    void mainLoop();
    void cleanup();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void initializePlayer();

    void checkGroundContact();
};
