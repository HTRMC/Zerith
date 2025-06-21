#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <array>
#include <set>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <spng.h>
#include <cstring>

// Logger and thread pool
#include "logger.h"
#include "thread_pool.h"
#include "profiler.h"

// Blockbench model support
#include "blockbench_model.h"
#include "blockbench_parser.h"
#include "blockbench_instance_generator.h"

// Chunk support
#include "chunk.h"
#include "chunk_mesh_generator.h"
#include "indirect_draw.h"
#include "chunk_manager.h"

// Player and collision support
#include "player.h"
#include "aabb.h"
#include "raycast.h"

// ImGui integration
#include "imgui_integration.h"

// Texture data structure
struct TextureData {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    std::vector<uint8_t> pixels;
};

// Function to load PNG using libspng
TextureData loadPNG(const std::string& filename) {
    PROFILE_FUNCTION();
    LOG_DEBUG("Loading PNG texture: %s", filename.c_str());
    TextureData texture;

    // Open the PNG file
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        LOG_ERROR("Failed to open PNG file: %s", filename.c_str());
        throw std::runtime_error("Failed to open PNG file: " + filename);
    }

    // Create a context
    spng_ctx* ctx = spng_ctx_new(0);
    if (!ctx) {
        fclose(fp);
        throw std::runtime_error("Failed to create spng context");
    }

    // Set the input file
    spng_set_png_file(ctx, fp);

    // Set CRC action to fix possible CRC errors
    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

    // Get image info
    spng_ihdr ihdr;
    int ret = spng_get_ihdr(ctx, &ihdr);
    if (ret) {
        spng_ctx_free(ctx);
        fclose(fp);
        throw std::runtime_error("Failed to get PNG header: " + std::string(spng_strerror(ret)));
    }

    texture.width = ihdr.width;
    texture.height = ihdr.height;
    
    // Print image info for debugging
    LOG_TRACE("PNG Info: %s - Width: %u, Height: %u, Bit depth: %d, Color type: %d", 
              filename.c_str(), ihdr.width, ihdr.height, (int)ihdr.bit_depth, (int)ihdr.color_type);
    
    // Check if the image has transparency information (tRNS chunk for indexed color)
    if (ihdr.color_type == SPNG_COLOR_TYPE_INDEXED) {
        struct spng_trns trns = {0};
        ret = spng_get_trns(ctx, &trns);
        if (ret == 0) {
            LOG_TRACE("PNG has tRNS chunk with %u transparent palette entries", trns.n_type3_entries);
        }
    }
    
    // Always decode to RGBA8 for consistency with Vulkan
    int fmt = SPNG_FMT_RGBA8;
    texture.channels = 4;  // Always use 4 channels (RGBA)
    
    // Calculate output size
    size_t out_size;
    ret = spng_decoded_image_size(ctx, fmt, &out_size);
    if (ret) {
        spng_ctx_free(ctx);
        fclose(fp);
        throw std::runtime_error("Failed to calculate output size: " + std::string(spng_strerror(ret)));
    }

    // Allocate buffer for the image
    texture.pixels.resize(out_size);

    // Decode the image with transparency support
    ret = spng_decode_image(ctx, texture.pixels.data(), out_size, fmt, SPNG_DECODE_TRNS);
    if (ret) {
        spng_ctx_free(ctx);
        fclose(fp);
        throw std::runtime_error("Failed to decode image: " + std::string(spng_strerror(ret)));
    }

    // Clean up
    spng_ctx_free(ctx);
    fclose(fp);
    
    
    return texture;
}

// Helper function to save a debug image in PPM format (a simple uncompressed format)
void saveDebugImage(const TextureData& texture, const std::string& filename) {
    // Only save if we have pixel data
    if (texture.pixels.empty() || texture.width == 0 || texture.height == 0) {
        LOG_ERROR("Cannot save debug image: No valid pixel data");
        return;
    }
    
    // Open output file
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        LOG_ERROR("Failed to open file for writing debug image: %s", filename.c_str());
        return;
    }
    
    // Write PPM header
    fprintf(fp, "P6\n%d %d\n255\n", texture.width, texture.height);
    
    // Write pixel data (convert RGBA to RGB for PPM)
    for (uint32_t y = 0; y < texture.height; y++) {
        for (uint32_t x = 0; x < texture.width; x++) {
            // Get pixel position in our buffer (RGBA format)
            size_t pixelIndex = (y * texture.width + x) * texture.channels;
            
            // Write only RGB components (PPM doesn't support alpha)
            fputc(texture.pixels[pixelIndex + 0], fp); // R
            fputc(texture.pixels[pixelIndex + 1], fp); // G
            fputc(texture.pixels[pixelIndex + 2], fp); // B
        }
    }
    
    fclose(fp);
    LOG_DEBUG("Debug image saved to: %s", filename.c_str());
}

// Constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

// Validation layers for debugging
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Device extensions
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_MESH_SHADER_EXTENSION_NAME  // Mesh shader extension
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Uniform buffer object
struct UniformBufferObject {
    // Time value for animation (4 bytes)
    alignas(4) float time;
    
    // View matrix (64 bytes)
    alignas(16) glm::mat4 view;
    
    // Projection matrix (64 bytes)
    alignas(16) glm::mat4 proj;

    // Number of face instances to render (4 bytes)
    alignas(4) uint32_t faceCount;
    
    // Camera position for LOD (16 bytes)
    alignas(16) glm::vec4 cameraPos;
    
    // Frustum planes for GPU culling (6 planes * 16 bytes = 96 bytes)
    // Each plane is (normal.x, normal.y, normal.z, distance)
    alignas(16) glm::vec4 frustumPlanes[6];

    // Total: 136 bytes (4 + 64 + 64 + 4)
};

// Grass texture indices uniform buffer
struct GrassTextureIndices {
    alignas(4) uint32_t grassTopLayer;      // Layer index for grass_block_top
    alignas(4) uint32_t grassSideLayer;     // Layer index for grass_block_side
    alignas(4) uint32_t grassOverlayLayer;  // Layer index for grass_block_side_overlay
    alignas(4) uint32_t padding;            // Padding for alignment
};

// Transparency texture indices uniform buffer
struct TransparencyTextureIndices {
    alignas(4) uint32_t waterLayer;         // Layer index for water texture
    alignas(4) uint32_t glassLayer;         // Layer index for glass texture
    alignas(4) uint32_t leavesLayer;        // Layer index for oak_leaves texture
    alignas(4) uint32_t padding;            // Padding for alignment
};

// Face instance data for storage buffer
struct FaceInstanceData {
    alignas(16) glm::vec4 position; // vec3 + padding
    alignas(16) glm::vec4 rotation; // quaternion
    alignas(16) glm::vec4 scale;    // face scale (width, height, 1.0, faceDirection)
    alignas(16) glm::vec4 uv;       // UV coordinates [minU, minV, maxU, maxV]
    uint32_t textureLayer;          // Texture array layer index
    uint32_t padding[3];            // Padding to maintain 16-byte alignment
};


// Queue family indices helper
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// Swap chain support details helper
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Helper function to check validation layer support
bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
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

// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            LOG_ERROR("Vulkan validation layer: %s", pCallbackData->pMessage);
        } else {
            LOG_WARN("Vulkan validation layer: %s", pCallbackData->pMessage);
        }
    }

    return VK_FALSE;
}

// Main application class
class ZerithApplication {
public:
    void run() {
        initWindow();
        loadBlockbenchModel();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    
    // Render layer pipelines
    VkPipeline opaquePipeline = VK_NULL_HANDLE;
    VkPipeline cutoutPipeline = VK_NULL_HANDLE;
    VkPipeline translucentPipeline = VK_NULL_HANDLE;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    
    // Grass texture indices uniform buffers
    std::vector<VkBuffer> grassTextureBuffers;
    std::vector<VkDeviceMemory> grassTextureBuffersMemory;
    std::vector<void*> grassTextureBuffersMapped;
    
    std::vector<VkBuffer> transparencyTextureBuffers;
    std::vector<VkDeviceMemory> transparencyTextureBuffersMemory;
    std::vector<void*> transparencyTextureBuffersMapped;

    // Storage buffer for face instances
    VkBuffer faceInstanceBuffer;
    VkDeviceMemory faceInstanceBufferMemory;
    
    // Layered face instance buffers
    VkBuffer opaqueInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory opaqueInstanceBufferMemory = VK_NULL_HANDLE;
    VkBuffer cutoutInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory cutoutInstanceBufferMemory = VK_NULL_HANDLE;
    VkBuffer translucentInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory translucentInstanceBufferMemory = VK_NULL_HANDLE;
    
    // Indirect draw buffers
    VkBuffer indirectDrawBuffer;
    VkDeviceMemory indirectDrawBufferMemory;
    VkBuffer chunkDataBuffer;
    VkDeviceMemory chunkDataBufferMemory;
    void* faceInstanceBufferMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // Texture resources
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;
    
    // Depth buffer resources
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // Mesh shader specific function pointers
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT;
    PFN_vkCmdDrawMeshTasksIndirectEXT vkCmdDrawMeshTasksIndirectEXT;

    bool framebufferResized = false;

    size_t currentFrame = 0;
    
    // Player with collision detection
    std::unique_ptr<Zerith::Player> player;
    
    // Input state tracking
    bool keysPressed[348] = { false };  // GLFW supports up to KEY_LAST (348)
    
    // Blockbench model support
    BlockbenchModel::Model currentModel;
    BlockbenchInstanceGenerator::ModelInstances currentInstances;
    
    // Layered face instances for proper transparency rendering
    struct LayeredInstances {
        std::vector<BlockbenchInstanceGenerator::FaceInstance> opaque;
        std::vector<BlockbenchInstanceGenerator::FaceInstance> cutout;
        std::vector<BlockbenchInstanceGenerator::FaceInstance> translucent;
    } layeredInstances;
    
    // Chunk support
    std::unique_ptr<Zerith::ChunkManager> chunkManager;
    
    // Debug data
    UniformBufferObject lastUBO{};
    
    // AABB Debug Rendering
    VkPipelineLayout aabbPipelineLayout;
    VkPipeline aabbDebugPipeline;
    VkDescriptorSetLayout aabbDescriptorSetLayout;
    std::vector<VkDescriptorSet> aabbDescriptorSets;
    VkBuffer aabbInstanceBuffer;
    VkDeviceMemory aabbInstanceBufferMemory;
    void* aabbInstanceBufferMapped;
    std::unique_ptr<Zerith::AABBDebugRenderer> aabbDebugRenderer;
    bool showDebugAABBs = false;
    bool mouseCaptured = true;
    
    // ImGui integration
    ImGuiIntegration imguiIntegration;

    // Calculate maximum chunks based on render distance
    // Account for the fact that chunks are loaded in a cube around the player
    size_t calculateMaxChunks(int renderDistance) {
        // Calculate chunks in each dimension (2 * renderDistance + 1)
        int chunksPerDimension = 2 * renderDistance + 1;
        
        // For now, assume cubic loading pattern
        // In practice, Y dimension might be limited by world height
        size_t maxChunks = chunksPerDimension * chunksPerDimension * chunksPerDimension;
        
        // Add some buffer for safety (20% extra)
        maxChunks = static_cast<size_t>(maxChunks * 1.2);
        
        // Ensure minimum size
        return std::max(maxChunks, size_t(1000));
    }

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Zerith", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        
        // Set up callbacks
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        glfwSetScrollCallback(window, scrollCallback);
        
        // Capture mouse cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Initialize player
        player = std::make_unique<Zerith::Player>(glm::vec3(0.0f, 70.0f, 0.0f));
        
        // Initialize AABB debug renderer
        aabbDebugRenderer = std::make_unique<Zerith::AABBDebugRenderer>();
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<ZerithApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
    
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto app = reinterpret_cast<ZerithApplication*>(glfwGetWindowUserPointer(window));
        
        // Only track keys we care about (prevents array access issues)
        if (key >= 0 && key < 348) {
            if (action == GLFW_PRESS) {
                app->keysPressed[key] = true;
            } else if (action == GLFW_RELEASE) {
                app->keysPressed[key] = false;
            }
        }
        
        // Toggle mouse capture with escape key
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            app->mouseCaptured = !app->mouseCaptured;
            if (app->mouseCaptured) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                LOG_INFO("Mouse capture: ON");
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                LOG_INFO("Mouse capture: OFF");
            }
        }
        
        // Toggle AABB debug rendering with F3
        if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
            app->showDebugAABBs = !app->showDebugAABBs;
            LOG_INFO("AABB debug rendering: %s", app->showDebugAABBs ? "ON" : "OFF");
        }
    }
    
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
        // Mouse input is now handled directly by the Player class in handleInput()
    }
    
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto app = reinterpret_cast<ZerithApplication*>(glfwGetWindowUserPointer(window));
        if (app->player) {
            app->player->handleScrollInput(xoffset, yoffset);
        }
    }
    
    void loadBlockbenchModel() {
        LOG_INFO("Creating chunk world...");
        
        // Initialize chunk manager
        chunkManager = std::make_unique<Zerith::ChunkManager>();
        
        // Set timing callbacks for ImGui performance tracking
        chunkManager->setTimingCallbacks(
            [this](float time) { imguiIntegration.updateChunkGenTime(time); },
            [this](float time) { imguiIntegration.updateMeshGenTime(time); }
        );
        
        // Set initial render distance
        chunkManager->setRenderDistance(2); // Start with 2 chunks render distance
        
        // Don't update chunks yet - wait until after Vulkan is initialized
        LOG_INFO("Chunk manager initialized");
    }
    
    void updateChunks() {
        if (!chunkManager) return;
        
        // Store previous face count
        size_t previousFaceCount = currentInstances.faces.size();
        
        // Update loaded chunks based on player position
        chunkManager->updateLoadedChunks(player ? player->getPosition() : glm::vec3(0.0f));
        
        // Only update face instances if they have changed (avoids unnecessary operations)
        if (chunkManager->hasFaceInstancesChanged()) {
            // Get face instances for rendering (only when changed)
            auto newFaces = chunkManager->getFaceInstancesWhenChanged();
            currentInstances.faces = std::move(newFaces);
            
            // Separate faces into render layers
            separateFacesByRenderLayer();
            
            // Recreate buffer since face instances changed
            recreateFaceInstanceBuffer();
            recreateLayeredInstanceBuffers();
        }
    }
    
    void separateFacesByRenderLayer() {
        // Clear previous layered instances
        layeredInstances.opaque.clear();
        layeredInstances.cutout.clear();
        layeredInstances.translucent.clear();
        
        if (!chunkManager) return;
        
        // For now, we need to classify faces based on the block registry
        // since we don't have access to per-face block type information
        // This is a temporary implementation - ideally faces would be 
        // pre-separated during chunk mesh generation
        
        // Get all chunks and their face instances
        const auto& allFaces = currentInstances.faces;
        
        // Separate faces based on texture layer (temporary hack)
        // This is a simplified approach - proper implementation would need block type per face
        auto meshGenerator = chunkManager->getMeshGenerator();
        auto textureArray = meshGenerator->getTextureArray();
        
        // Use the render layer information directly from the face instances
        for (const auto& face : allFaces) {
            switch (face.renderLayer) {
                case Zerith::RenderLayer::OPAQUE:
                    layeredInstances.opaque.push_back(face);
                    break;
                case Zerith::RenderLayer::CUTOUT:
                    layeredInstances.cutout.push_back(face);
                    break;
                case Zerith::RenderLayer::TRANSLUCENT:
                    layeredInstances.translucent.push_back(face);
                    break;
                default:
                    // Default to opaque for unknown layers
                    layeredInstances.opaque.push_back(face);
                    break;
            }
        }
        
        // LOG_INFO("Separated %zu faces: %zu opaque, %zu cutout, %zu translucent",
        //          allFaces.size(),
        //          layeredInstances.opaque.size(),
        //          layeredInstances.cutout.size(),
        //          layeredInstances.translucent.size());
        
        // Debug: count faces by render layer from face instances
        int opaqueCount = 0, cutoutCount = 0, translucentCount = 0;
        for (const auto& face : allFaces) {
            switch (face.renderLayer) {
                case Zerith::RenderLayer::OPAQUE: opaqueCount++; break;
                case Zerith::RenderLayer::CUTOUT: cutoutCount++; break;
                case Zerith::RenderLayer::TRANSLUCENT: translucentCount++; break;
            }
        }
        // LOG_INFO("Face instances by render layer: %d opaque, %d cutout, %d translucent",
        //          opaqueCount, cutoutCount, translucentCount);
    }
    
    void recreateFaceInstanceBuffer() {
        // Wait for device to be idle
        vkDeviceWaitIdle(device);
        
        // Clean up old buffer
        if (faceInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, faceInstanceBuffer, nullptr);
            vkFreeMemory(device, faceInstanceBufferMemory, nullptr);
        }
        
        // Create new buffer
        createFaceInstanceBuffer();
        
        // Update descriptor sets
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkDescriptorBufferInfo storageBufferInfo{};
            storageBufferInfo.buffer = faceInstanceBuffer;
            storageBufferInfo.offset = 0;
            storageBufferInfo.range = sizeof(FaceInstanceData) * currentInstances.faces.size();

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 2;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &storageBufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }
    
    void createDefaultCube() {
        LOG_DEBUG("Creating default cube model");
        
        // Create a simple default cube element
        BlockbenchModel::Element element;
        element.from = glm::vec3(0.0f, 0.0f, 0.0f);
        element.to = glm::vec3(16.0f, 16.0f, 16.0f);
        
        // Set up all faces with default texture references
        element.down.texture = "#down";
        element.up.texture = "#up";
        element.north.texture = "#north";
        element.south.texture = "#south";
        element.west.texture = "#west";
        element.east.texture = "#east";
        
        currentModel.elements.push_back(element);
        currentInstances = BlockbenchInstanceGenerator::Generator::generateModelInstances(currentModel);

        LOG_DEBUG("Default cube created with %zu faces (including green origin dot)", currentInstances.faces.size());
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createDepthResources();
        createRenderPass();
        initImGui();
        createDescriptorSetLayout();
        createCommandPool();  // Create command pool earlier
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createGraphicsPipeline();
        createLayeredRenderPipelines();
        createAABBDebugPipeline();
        createFramebuffers();
        createUniformBuffers();
        createFaceInstanceBuffer();
        createLayeredInstanceBuffers();
        createAABBInstanceBuffer();
        createIndirectDrawBuffers(); // Re-enabled for proper indirect drawing
        createDescriptorPool();
        createDescriptorSets();
        createAABBDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
        
        // Now that Vulkan is initialized, we can update chunks
        updateChunks();
        LOG_INFO("World initialized with %zu chunks", chunkManager->getLoadedChunkCount());
    }
    
    // Create a default checkerboard texture if PNG loading fails
    TextureData createDefaultTexture() {
        TextureData texture;
        texture.width = 64;
        texture.height = 64;
        texture.channels = 4; // RGBA
        
        // Create a 64x64 RGBA checkerboard texture
        texture.pixels.resize(texture.width * texture.height * texture.channels);
        
        for (uint32_t y = 0; y < texture.height; y++) {
            for (uint32_t x = 0; x < texture.width; x++) {
                uint32_t index = (y * texture.width + x) * texture.channels;
                
                // 8x8 checkerboard pattern
                bool isCheckerboardPattern = ((x / 8) % 2) != ((y / 8) % 2);
                
                // Set color based on checkerboard pattern
                if (isCheckerboardPattern) {
                    // White square
                    texture.pixels[index + 0] = 255; // R
                    texture.pixels[index + 1] = 255; // G
                    texture.pixels[index + 2] = 255; // B
                    texture.pixels[index + 3] = 255; // A
                } else {
                    // Gray square
                    texture.pixels[index + 0] = 120; // R
                    texture.pixels[index + 1] = 120; // G
                    texture.pixels[index + 2] = 120; // B
                    texture.pixels[index + 3] = 255; // A
                }
            }
        }
        
        return texture;
    }

    void createTextureImage() {
        // Get texture array manager from chunk manager
        auto textureArray = chunkManager->getMeshGenerator()->getTextureArray();
        const auto& textureFiles = textureArray->getTextureFiles();
        uint32_t layerCount = static_cast<uint32_t>(textureFiles.size());
        
        // Load all textures
        std::vector<TextureData> textures;
        for (const auto& filename : textureFiles) {
            try {
                TextureData textureData;
                // Try both paths - for running from source dir or from build dir
                try {
                    textureData = loadPNG(filename);
                    LOG_DEBUG("Loaded texture: %s", filename.c_str());
                } catch (const std::runtime_error&) {
                    textureData = loadPNG("../" + filename);
                    LOG_DEBUG("Loaded texture: ../%s", filename.c_str());
                }
                textures.push_back(textureData);
            } catch (const std::runtime_error& e) {
                LOG_WARN("Failed to load texture %s, using default: %s", filename.c_str(), e.what());
                textures.push_back(createDefaultTexture());
            }
        }
        
        // Assume all textures are the same size (16x16)
        uint32_t textureWidth = 16;
        uint32_t textureHeight = 16;
        uint32_t textureChannels = 4; // RGBA
        VkDeviceSize layerSize = textureWidth * textureHeight * textureChannels;
        VkDeviceSize totalSize = layerSize * layerCount;
        
        // Create a staging buffer for all texture layers
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        createBuffer(
            totalSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, 
            stagingBufferMemory
        );
        
        // Copy all texture data to staging buffer
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, totalSize, 0, &data);
        
        for (uint32_t i = 0; i < layerCount; ++i) {
            void* layerData = static_cast<uint8_t*>(data) + (i * layerSize);
            memcpy(layerData, textures[i].pixels.data(), static_cast<size_t>(layerSize));
        }
        
        vkUnmapMemory(device, stagingBufferMemory);
        
        // Create texture array image
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = textureWidth;
        imageInfo.extent.height = textureHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = layerCount;  // This makes it a texture array
        imageInfo.format = imageFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture array image!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate texture array image memory!");
        }
        
        vkBindImageMemory(device, textureImage, textureImageMemory, 0);
        
        // Transition image layout for copy operation (all layers)
        transitionImageLayoutArray(
            textureImage, 
            imageFormat, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            layerCount
        );
        
        // Copy from staging buffer to texture array
        copyBufferToImageArray(
            stagingBuffer, 
            textureImage, 
            textureWidth, 
            textureHeight,
            layerCount
        );
        
        // Transition image layout for shader access (all layers)
        transitionImageLayoutArray(
            textureImage, 
            imageFormat, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            layerCount
        );
        
        // Clean up staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                    VkImage& image, VkDeviceMemory& imageMemory) {
        // Create image
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
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }
        
        // Allocate memory for the image
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
    
    VkCommandBuffer beginSingleTimeCommands() {
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
    
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
        barrier.subresourceRange.layerCount = 1;
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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
    
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
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
        
        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
        
        endSingleTimeCommands(commandBuffer);
    }
    
    void transitionImageLayoutArray(VkImage image, VkFormat format, VkImageLayout oldLayout, 
                                   VkImageLayout newLayout, uint32_t layerCount) {
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
        barrier.subresourceRange.layerCount = layerCount; // All layers
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
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
    
    void copyBufferToImageArray(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        
        std::vector<VkBufferImageCopy> regions;
        VkDeviceSize layerSize = width * height * 4; // 4 bytes per pixel (RGBA)
        
        for (uint32_t layer = 0; layer < layerCount; ++layer) {
            VkBufferImageCopy region{};
            region.bufferOffset = layer * layerSize;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = layer;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {width, height, 1};
            
            regions.push_back(region);
        }
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                              static_cast<uint32_t>(regions.size()), regions.data());
        
        endSingleTimeCommands(commandBuffer);
    }
    
    void createTextureImageView() {
        // Get layer count from texture array
        auto textureArray = chunkManager->getMeshGenerator()->getTextureArray();
        uint32_t layerCount = static_cast<uint32_t>(textureArray->getLayerCount());
        
        textureImageView = createImageViewArray(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, layerCount);
    }
    
    VkImageView createImageViewArray(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layerCount) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // Array view type
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount; // All layers
        
        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture array image view!");
        }
        
        return imageView;
    }
    
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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
            throw std::runtime_error("failed to create texture image view!");
        }
        
        return imageView;
    }
    
    void createTextureSampler() {
        // Get device features to check for anisotropy support
        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
        
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST; // Changed to NEAREST for pixelated look
        samplerInfo.minFilter = VK_FILTER_NEAREST; // Changed to NEAREST for pixelated look
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        
        // Only enable anisotropy if the device supports it
        samplerInfo.anisotropyEnable = deviceFeatures.samplerAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = deviceFeatures.samplerAnisotropy ? 
                                   properties.limits.maxSamplerAnisotropy : 1.0f;
        
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        
        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Mesh Shader Cube";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        // Check for mesh shader support
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

        // Check for maintenance4 feature support
        VkPhysicalDeviceMaintenance4Features maintenance4Features{};
        maintenance4Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
        maintenance4Features.pNext = nullptr;
        
        // Chain the structs for the query
        meshShaderFeatures.pNext = &maintenance4Features;

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &meshShaderFeatures;

        vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

        // Display feature support info
        LOG_INFO("Device features:");
        LOG_INFO("  - Mesh shader: %s", meshShaderFeatures.meshShader ? "supported" : "not supported");
        LOG_INFO("  - Task shader: %s", meshShaderFeatures.taskShader ? "supported" : "not supported");
        LOG_INFO("  - Maintenance4: %s", maintenance4Features.maintenance4 ? "supported" : "not supported");

        return indices.isComplete() && extensionsSupported && swapChainAdequate &&
               meshShaderFeatures.meshShader && meshShaderFeatures.taskShader && 
               maintenance4Features.maintenance4; // Now also require maintenance4
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
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

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
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

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Get available physical device features
        VkPhysicalDeviceFeatures availableFeatures{};
        vkGetPhysicalDeviceFeatures(physicalDevice, &availableFeatures);
        
        // Enable the features we want to use
        VkPhysicalDeviceFeatures deviceFeatures{};
        
        // Enable anisotropic filtering if available
        if (availableFeatures.samplerAnisotropy) {
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            LOG_INFO("Anisotropic filtering enabled");
        } else {
            LOG_WARN("Anisotropic filtering not available");
        }
        
        // Enable fillModeNonSolid for wireframe rendering
        if (availableFeatures.fillModeNonSolid) {
            deviceFeatures.fillModeNonSolid = VK_TRUE;
            LOG_INFO("Fill mode non-solid enabled");
        } else {
            LOG_WARN("Fill mode non-solid not available");
        }
        
        // Enable wideLines for line width > 1.0
        if (availableFeatures.wideLines) {
            deviceFeatures.wideLines = VK_TRUE;
            LOG_INFO("Wide lines enabled");
        } else {
            LOG_WARN("Wide lines not available");
        }

        // Enable mesh shader features
        VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        meshShaderFeatures.taskShader = VK_TRUE;
        meshShaderFeatures.meshShader = VK_TRUE;
        
        // Check for and enable maintenance4 feature (needed for LocalSizeId execution mode in SPIR-V)
        VkPhysicalDeviceMaintenance4Features availableMaintenance4Features{};
        availableMaintenance4Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
        availableMaintenance4Features.pNext = nullptr;
        
        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &availableMaintenance4Features;
        
        vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
        
        VkPhysicalDeviceMaintenance4Features maintenance4Features{};
        maintenance4Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
        maintenance4Features.pNext = nullptr;
        
        if (availableMaintenance4Features.maintenance4) {
            maintenance4Features.maintenance4 = VK_TRUE;
            LOG_INFO("Maintenance4 feature enabled");
        } else {
            maintenance4Features.maintenance4 = VK_FALSE;
            LOG_WARN("Maintenance4 feature not available, shader may not work properly");
        }
        
        // Chain the feature structs
        meshShaderFeatures.pNext = &maintenance4Features;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &meshShaderFeatures;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

        // Load mesh shader extension functions
        vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(
            vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT"));

        if (!vkCmdDrawMeshTasksEXT) {
            throw std::runtime_error("failed to get mesh shader function pointer!");
        }
        
        vkCmdDrawMeshTasksIndirectEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(
            vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectEXT"));
        
        if (!vkCmdDrawMeshTasksIndirectEXT) {
            throw std::runtime_error("failed to get indirect mesh shader function pointer!");
        }
    }

    void createSwapChain() {
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
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createImageViews() {
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
    
    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        
        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                   depthImage, depthImageMemory);
        
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
    
    VkFormat findDepthFormat() {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }
    
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
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

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
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
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void initImGui() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        
        if (!imguiIntegration.initialize(window, instance, physicalDevice, device, 
                                       indices.graphicsFamily.value(), graphicsQueue, 
                                       renderPass, swapChainImages.size(), swapChainImages.size())) {
            throw std::runtime_error("failed to initialize ImGui!");
        }
    }

    void createDescriptorSetLayout() {
        // UBO binding for mesh shader
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        
        // Texture sampler binding for fragment shader
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        
        // Storage buffer binding for face instances
        VkDescriptorSetLayoutBinding storageLayoutBinding{};
        storageLayoutBinding.binding = 2;
        storageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageLayoutBinding.descriptorCount = 1;
        storageLayoutBinding.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
        storageLayoutBinding.pImmutableSamplers = nullptr;
        
        // Storage buffer binding for chunk data (for indirect drawing)
        VkDescriptorSetLayoutBinding chunkDataLayoutBinding{};
        chunkDataLayoutBinding.binding = 3;
        chunkDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        chunkDataLayoutBinding.descriptorCount = 1;
        chunkDataLayoutBinding.stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT;
        chunkDataLayoutBinding.pImmutableSamplers = nullptr;

        // Grass texture indices binding for fragment shader  
        VkDescriptorSetLayoutBinding grassTexturesLayoutBinding{};
        grassTexturesLayoutBinding.binding = 4;
        grassTexturesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        grassTexturesLayoutBinding.descriptorCount = 1;
        grassTexturesLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        grassTexturesLayoutBinding.pImmutableSamplers = nullptr;

        // Transparency texture indices binding for fragment shader  
        VkDescriptorSetLayoutBinding transparencyTexturesLayoutBinding{};
        transparencyTexturesLayoutBinding.binding = 5;
        transparencyTexturesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        transparencyTexturesLayoutBinding.descriptorCount = 1;
        transparencyTexturesLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        transparencyTexturesLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 6> bindings = {uboLayoutBinding, samplerLayoutBinding, storageLayoutBinding, chunkDataLayoutBinding, grassTexturesLayoutBinding, transparencyTexturesLayoutBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
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

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createGraphicsPipeline() {
        // Load shader code from compiled spv files
        auto taskShaderCode = readFile("shaders/task_shader.spv");
        auto meshShaderCode = readFile("shaders/mesh_shader.spv");
        auto fragShaderCode = readFile("shaders/fragment_shader.spv");

        VkShaderModule taskShaderModule = createShaderModule(taskShaderCode);
        VkShaderModule meshShaderModule = createShaderModule(meshShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo taskShaderStageInfo{};
        taskShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        taskShaderStageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
        taskShaderStageInfo.module = taskShaderModule;
        taskShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo meshShaderStageInfo{};
        meshShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        meshShaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
        meshShaderStageInfo.module = meshShaderModule;
        meshShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {
            taskShaderStageInfo,
            meshShaderStageInfo,
            fragShaderStageInfo
        };

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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

        // Add depth stencil state for proper depth testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Push constant for render layer
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(uint32_t);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 3;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, meshShaderModule, nullptr);
        vkDestroyShaderModule(device, taskShaderModule, nullptr);
    }
    
    void createLayeredRenderPipelines() {
        LOG_INFO("Creating layered render pipelines...");
        // Load shader code from compiled spv files
        auto taskShaderCode = readFile("shaders/task_shader.spv");
        auto meshShaderCode = readFile("shaders/mesh_shader.spv");
        auto fragShaderCode = readFile("shaders/fragment_shader.spv");

        VkShaderModule taskShaderModule = createShaderModule(taskShaderCode);
        VkShaderModule meshShaderModule = createShaderModule(meshShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo taskShaderStageInfo{};
        taskShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        taskShaderStageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
        taskShaderStageInfo.module = taskShaderModule;
        taskShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo meshShaderStageInfo{};
        meshShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        meshShaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
        meshShaderStageInfo.module = meshShaderModule;
        meshShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {
            taskShaderStageInfo,
            meshShaderStageInfo,
            fragShaderStageInfo
        };

        // Shared pipeline state
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // OPAQUE PIPELINE - No blending, depth write enabled
        VkPipelineColorBlendAttachmentState opaqueBlendAttachment{};
        opaqueBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        opaqueBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo opaqueBlending{};
        opaqueBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        opaqueBlending.logicOpEnable = VK_FALSE;
        opaqueBlending.attachmentCount = 1;
        opaqueBlending.pAttachments = &opaqueBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo opaqueDepthStencil{};
        opaqueDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        opaqueDepthStencil.depthTestEnable = VK_TRUE;
        opaqueDepthStencil.depthWriteEnable = VK_TRUE;
        opaqueDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        opaqueDepthStencil.depthBoundsTestEnable = VK_FALSE;
        opaqueDepthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo opaquePipelineInfo{};
        opaquePipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        opaquePipelineInfo.stageCount = 3;
        opaquePipelineInfo.pStages = shaderStages;
        opaquePipelineInfo.pViewportState = &viewportState;
        opaquePipelineInfo.pRasterizationState = &rasterizer;
        opaquePipelineInfo.pMultisampleState = &multisampling;
        opaquePipelineInfo.pDepthStencilState = &opaqueDepthStencil;
        opaquePipelineInfo.pColorBlendState = &opaqueBlending;
        opaquePipelineInfo.pDynamicState = &dynamicState;
        opaquePipelineInfo.layout = pipelineLayout;
        opaquePipelineInfo.renderPass = renderPass;
        opaquePipelineInfo.subpass = 0;
        opaquePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &opaquePipelineInfo, nullptr, &opaquePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create opaque pipeline!");
        }
        LOG_INFO("Opaque pipeline created successfully");

        // CUTOUT PIPELINE - No blending (uses alpha testing in shader), depth write enabled
        VkPipelineColorBlendAttachmentState cutoutBlendAttachment{};
        cutoutBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cutoutBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo cutoutBlending{};
        cutoutBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cutoutBlending.logicOpEnable = VK_FALSE;
        cutoutBlending.attachmentCount = 1;
        cutoutBlending.pAttachments = &cutoutBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo cutoutDepthStencil{};
        cutoutDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        cutoutDepthStencil.depthTestEnable = VK_TRUE;
        cutoutDepthStencil.depthWriteEnable = VK_TRUE;
        cutoutDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        cutoutDepthStencil.depthBoundsTestEnable = VK_FALSE;
        cutoutDepthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo cutoutPipelineInfo{};
        cutoutPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        cutoutPipelineInfo.stageCount = 3;
        cutoutPipelineInfo.pStages = shaderStages;
        cutoutPipelineInfo.pViewportState = &viewportState;
        cutoutPipelineInfo.pRasterizationState = &rasterizer;
        cutoutPipelineInfo.pMultisampleState = &multisampling;
        cutoutPipelineInfo.pDepthStencilState = &cutoutDepthStencil;
        cutoutPipelineInfo.pColorBlendState = &cutoutBlending;
        cutoutPipelineInfo.pDynamicState = &dynamicState;
        cutoutPipelineInfo.layout = pipelineLayout;
        cutoutPipelineInfo.renderPass = renderPass;
        cutoutPipelineInfo.subpass = 0;
        cutoutPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &cutoutPipelineInfo, nullptr, &cutoutPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create cutout pipeline!");
        }
        LOG_INFO("Cutout pipeline created successfully");

        // TRANSLUCENT PIPELINE - Alpha blending enabled, depth write disabled
        VkPipelineColorBlendAttachmentState translucentBlendAttachment{};
        translucentBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        translucentBlendAttachment.blendEnable = VK_TRUE;
        translucentBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        translucentBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        translucentBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        translucentBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        translucentBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        translucentBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo translucentBlending{};
        translucentBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        translucentBlending.logicOpEnable = VK_FALSE;
        translucentBlending.attachmentCount = 1;
        translucentBlending.pAttachments = &translucentBlendAttachment;

        VkPipelineDepthStencilStateCreateInfo translucentDepthStencil{};
        translucentDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        translucentDepthStencil.depthTestEnable = VK_TRUE;
        translucentDepthStencil.depthWriteEnable = VK_FALSE; // Don't write to depth buffer
        translucentDepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        translucentDepthStencil.depthBoundsTestEnable = VK_FALSE;
        translucentDepthStencil.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo translucentPipelineInfo{};
        translucentPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        translucentPipelineInfo.stageCount = 3;
        translucentPipelineInfo.pStages = shaderStages;
        translucentPipelineInfo.pViewportState = &viewportState;
        translucentPipelineInfo.pRasterizationState = &rasterizer;
        translucentPipelineInfo.pMultisampleState = &multisampling;
        translucentPipelineInfo.pDepthStencilState = &translucentDepthStencil;
        translucentPipelineInfo.pColorBlendState = &translucentBlending;
        translucentPipelineInfo.pDynamicState = &dynamicState;
        translucentPipelineInfo.layout = pipelineLayout;
        translucentPipelineInfo.renderPass = renderPass;
        translucentPipelineInfo.subpass = 0;
        translucentPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &translucentPipelineInfo, nullptr, &translucentPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create translucent pipeline!");
        }
        LOG_INFO("Translucent pipeline created successfully");

        // Clean up shader modules
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, meshShaderModule, nullptr);
        vkDestroyShaderModule(device, taskShaderModule, nullptr);
        
        LOG_INFO("All layered render pipelines created successfully");
    }
    
    void createAABBDebugPipeline() {
        // Create descriptor set layout for AABB debug
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
        
        VkDescriptorSetLayoutBinding storageLayoutBinding{};
        storageLayoutBinding.binding = 1;
        storageLayoutBinding.descriptorCount = 1;
        storageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageLayoutBinding.pImmutableSamplers = nullptr;
        storageLayoutBinding.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
        
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, storageLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &aabbDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create AABB descriptor set layout!");
        }
        
        // Load AABB shaders
        auto aabbMeshShaderCode = readFile("shaders/aabb_mesh_shader.spv");
        auto aabbFragShaderCode = readFile("shaders/aabb_fragment_shader.spv");
        
        VkShaderModule aabbMeshShaderModule = createShaderModule(aabbMeshShaderCode);
        VkShaderModule aabbFragShaderModule = createShaderModule(aabbFragShaderCode);
        
        VkPipelineShaderStageCreateInfo aabbMeshShaderStageInfo{};
        aabbMeshShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        aabbMeshShaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
        aabbMeshShaderStageInfo.module = aabbMeshShaderModule;
        aabbMeshShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo aabbFragShaderStageInfo{};
        aabbFragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        aabbFragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        aabbFragShaderStageInfo.module = aabbFragShaderModule;
        aabbFragShaderStageInfo.pName = "main";
        
        VkPipelineShaderStageCreateInfo aabbShaderStages[] = {
            aabbMeshShaderStageInfo,
            aabbFragShaderStageInfo
        };
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE; // Wireframe mode
        rasterizer.lineWidth = 2.0f; // Thicker lines for visibility
        rasterizer.cullMode = VK_CULL_MODE_NONE; // No culling for wireframes
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE; // Don't write depth for debug lines
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Draw on top
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &aabbDescriptorSetLayout;
        
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &aabbPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create AABB pipeline layout!");
        }
        
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = aabbShaderStages;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = aabbPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &aabbDebugPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create AABB graphics pipeline!");
        }
        
        vkDestroyShaderModule(device, aabbFragShaderModule, nullptr);
        vkDestroyShaderModule(device, aabbMeshShaderModule, nullptr);
    }

    void createFramebuffers() {
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

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(swapChainImages.size());
        uniformBuffersMemory.resize(swapChainImages.size());
        uniformBuffersMapped.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
        
        // Create grass texture indices uniform buffers
        VkDeviceSize grassBufferSize = sizeof(GrassTextureIndices);
        
        grassTextureBuffers.resize(swapChainImages.size());
        grassTextureBuffersMemory.resize(swapChainImages.size());
        grassTextureBuffersMapped.resize(swapChainImages.size());
        
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer(grassBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        grassTextureBuffers[i], grassTextureBuffersMemory[i]);

            vkMapMemory(device, grassTextureBuffersMemory[i], 0, grassBufferSize, 0, &grassTextureBuffersMapped[i]);
        }
        
        // Create transparency texture indices uniform buffers
        VkDeviceSize transparencyBufferSize = sizeof(TransparencyTextureIndices);
        
        transparencyTextureBuffers.resize(swapChainImages.size());
        transparencyTextureBuffersMemory.resize(swapChainImages.size());
        transparencyTextureBuffersMapped.resize(swapChainImages.size());
        
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer(transparencyBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        transparencyTextureBuffers[i], transparencyTextureBuffersMemory[i]);

            vkMapMemory(device, transparencyTextureBuffersMemory[i], 0, transparencyBufferSize, 0, &transparencyTextureBuffersMapped[i]);
        }
    }
    
    void createFaceInstanceBuffer() {
        // Handle empty face list
        if (currentInstances.faces.empty()) {
            // Create a minimal buffer with one dummy face to avoid validation errors
            currentInstances.faces.push_back(BlockbenchInstanceGenerator::FaceInstance());
        }
        
        // Calculate buffer size for all face instances
        VkDeviceSize bufferSize = sizeof(FaceInstanceData) * currentInstances.faces.size();
        
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            faceInstanceBuffer,
            faceInstanceBufferMemory
        );
        
        // Map and populate the buffer
        vkMapMemory(device, faceInstanceBufferMemory, 0, bufferSize, 0, &faceInstanceBufferMapped);
        
        // Copy face instance data
        FaceInstanceData* mappedData = static_cast<FaceInstanceData*>(faceInstanceBufferMapped);
        for (size_t i = 0; i < currentInstances.faces.size(); i++) {
            const auto& face = currentInstances.faces[i];
            mappedData[i].position = glm::vec4(face.position, 1.0f); // Add w component
            mappedData[i].rotation = face.rotation;
            mappedData[i].scale = glm::vec4(face.scale, static_cast<float>(face.faceDirection)); // Pack direction in w
            mappedData[i].uv = face.uv; // Copy UV coordinates
            mappedData[i].textureLayer = face.textureLayer; // Copy texture layer index
            
            // Print face position for debugging
            const char* directionNames[] = {"DOWN", "UP", "NORTH", "SOUTH", "WEST", "EAST"};
            const char* directionName = (face.faceDirection >= 0 && face.faceDirection < 6) ? 
                                       directionNames[face.faceDirection] : "UNKNOWN";
        }
        
        // LOG_DEBUG("Face instance buffer created with %zu instances (%zu bytes)", 
        //           currentInstances.faces.size(), bufferSize);
    }
    
    void createLayeredInstanceBuffers() {
        createLayeredInstanceBuffer(layeredInstances.opaque, opaqueInstanceBuffer, opaqueInstanceBufferMemory);
        createLayeredInstanceBuffer(layeredInstances.cutout, cutoutInstanceBuffer, cutoutInstanceBufferMemory);
        createLayeredInstanceBuffer(layeredInstances.translucent, translucentInstanceBuffer, translucentInstanceBufferMemory);
    }
    
    void createLayeredInstanceBuffer(const std::vector<BlockbenchInstanceGenerator::FaceInstance>& faces,
                                   VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        if (faces.empty()) {
            buffer = VK_NULL_HANDLE;
            bufferMemory = VK_NULL_HANDLE;
            return;
        }
        
        VkDeviceSize bufferSize = sizeof(FaceInstanceData) * faces.size();
        
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer,
            bufferMemory
        );
        
        // Map and populate the buffer
        void* bufferMapped;
        vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &bufferMapped);
        
        // Copy face instance data
        FaceInstanceData* mappedData = static_cast<FaceInstanceData*>(bufferMapped);
        for (size_t i = 0; i < faces.size(); i++) {
            const auto& face = faces[i];
            mappedData[i].position = glm::vec4(face.position, 1.0f);
            mappedData[i].rotation = face.rotation;
            mappedData[i].scale = glm::vec4(face.scale, static_cast<float>(face.faceDirection));
            mappedData[i].uv = face.uv;
            mappedData[i].textureLayer = face.textureLayer;
            mappedData[i].padding[0] = 0;
            mappedData[i].padding[1] = 0;
            mappedData[i].padding[2] = 0;
        }
        
        vkUnmapMemory(device, bufferMemory);
    }
    
    void recreateLayeredInstanceBuffers() {
        // Clean up old buffers
        if (opaqueInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, opaqueInstanceBuffer, nullptr);
            vkFreeMemory(device, opaqueInstanceBufferMemory, nullptr);
        }
        if (cutoutInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, cutoutInstanceBuffer, nullptr);
            vkFreeMemory(device, cutoutInstanceBufferMemory, nullptr);
        }
        if (translucentInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, translucentInstanceBuffer, nullptr);
            vkFreeMemory(device, translucentInstanceBufferMemory, nullptr);
        }
        
        // Create new buffers
        createLayeredInstanceBuffers();
    }
    
    void createAABBInstanceBuffer() {
        // Get current render distance to calculate buffer size
        int renderDistance = chunkManager ? chunkManager->getRenderDistance() : 8;
        size_t maxChunks = calculateMaxChunks(renderDistance);
        
        // Create buffer based on calculated max chunks
        VkDeviceSize bufferSize = sizeof(Zerith::AABBDebugData) * maxChunks;
        
        createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            aabbInstanceBuffer,
            aabbInstanceBufferMemory
        );
        
        // Map the buffer
        vkMapMemory(device, aabbInstanceBufferMemory, 0, bufferSize, 0, &aabbInstanceBufferMapped);
        
        LOG_DEBUG("AABB instance buffer created with capacity for %zu AABBs (render distance: %d)", maxChunks, renderDistance);
    }
    
    void createIndirectDrawBuffers() {
        // Get current render distance to calculate buffer size
        int renderDistance = chunkManager ? chunkManager->getRenderDistance() : 8;
        size_t maxChunks = calculateMaxChunks(renderDistance);
        
        // Create indirect draw command buffer
        VkDeviceSize indirectBufferSize = sizeof(Zerith::DrawMeshTasksIndirectCommand) * maxChunks;
        
        createBuffer(
            indirectBufferSize,
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            indirectDrawBuffer,
            indirectDrawBufferMemory
        );
        
        // Create chunk data buffer for GPU culling
        VkDeviceSize chunkDataSize = sizeof(Zerith::ChunkDrawData) * maxChunks;
        
        createBuffer(
            chunkDataSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            chunkDataBuffer,
            chunkDataBufferMemory
        );
        
        LOG_DEBUG("Indirect draw buffers created with capacity for %zu chunks", maxChunks);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * 3); // Triple for AABB + grass textures
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size() * 3); // Triple for AABB + chunk data

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size() * 2); // Double for AABB sets

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            // Uniform buffer info
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            
            // Image sampler info
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;
            
            // Storage buffer info
            VkDescriptorBufferInfo storageBufferInfo{};
            storageBufferInfo.buffer = faceInstanceBuffer;
            storageBufferInfo.offset = 0;
            storageBufferInfo.range = sizeof(FaceInstanceData) * currentInstances.faces.size();

            // Grass texture indices buffer info
            VkDescriptorBufferInfo grassTextureBufferInfo{};
            grassTextureBufferInfo.buffer = grassTextureBuffers[i];
            grassTextureBufferInfo.offset = 0;
            grassTextureBufferInfo.range = sizeof(GrassTextureIndices);

            // Transparency texture indices buffer info
            VkDescriptorBufferInfo transparencyTextureBufferInfo{};
            transparencyTextureBufferInfo.buffer = transparencyTextureBuffers[i];
            transparencyTextureBufferInfo.offset = 0;
            transparencyTextureBufferInfo.range = sizeof(TransparencyTextureIndices);

            std::array<VkWriteDescriptorSet, 6> descriptorWrites{};
            
            // Uniform buffer descriptor
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            
            // Texture sampler descriptor
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            
            // Storage buffer descriptor
            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &storageBufferInfo;
            
            // Chunk data buffer descriptor
            VkDescriptorBufferInfo chunkDataBufferInfo{};
            chunkDataBufferInfo.buffer = chunkDataBuffer;
            chunkDataBufferInfo.offset = 0;
            chunkDataBufferInfo.range = VK_WHOLE_SIZE;
            
            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo = &chunkDataBufferInfo;
            
            // Grass texture indices descriptor
            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = descriptorSets[i];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pBufferInfo = &grassTextureBufferInfo;
            
            // Transparency texture indices descriptor
            descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[5].dstSet = descriptorSets[i];
            descriptorWrites[5].dstBinding = 5;
            descriptorWrites[5].dstArrayElement = 0;
            descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[5].descriptorCount = 1;
            descriptorWrites[5].pBufferInfo = &transparencyTextureBufferInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
    
    void createAABBDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), aabbDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        aabbDescriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, aabbDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate AABB descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            // Uniform buffer info (reuse the same UBO)
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);
            
            // AABB storage buffer info
            VkDescriptorBufferInfo storageBufferInfo{};
            storageBufferInfo.buffer = aabbInstanceBuffer;
            storageBufferInfo.offset = 0;
            int renderDistance = chunkManager ? chunkManager->getRenderDistance() : 8;
            size_t maxChunks = calculateMaxChunks(renderDistance);
            storageBufferInfo.range = sizeof(Zerith::AABBDebugData) * maxChunks;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            
            // Uniform buffer descriptor
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = aabbDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            
            // Storage buffer descriptor
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = aabbDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &storageBufferInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void createSyncObjects() {
        // Create semaphores per swapchain image to avoid synchronization conflicts
        imageAvailableSemaphores.resize(swapChainImages.size());
        renderFinishedSemaphores.resize(swapChainImages.size());
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // Create semaphores for each swapchain image
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for swapchain image!");
            }
        }

        // Create fences for frames in flight
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void updateIndirectDrawBuffer() {
        if (!chunkManager) return;
        
        const auto& indirectManager = chunkManager->getIndirectDrawManager();
        const auto& drawCommands = indirectManager.getDrawCommands();
        
        if (drawCommands.empty()) return;
        
        // Update indirect draw buffer with all commands at once
        int renderDistance = chunkManager->getRenderDistance();
        size_t maxCommands = calculateMaxChunks(renderDistance);
        size_t commandCount = std::min(drawCommands.size(), maxCommands);
        
        void* data;
        vkMapMemory(device, indirectDrawBufferMemory, 0, 
                   sizeof(Zerith::DrawMeshTasksIndirectCommand) * commandCount, 0, &data);
        memcpy(data, drawCommands.data(), sizeof(Zerith::DrawMeshTasksIndirectCommand) * commandCount);
        vkUnmapMemory(device, indirectDrawBufferMemory);
    }
    
    // Extract frustum planes from view-projection matrix
    void extractFrustumPlanes(const glm::mat4& vp, glm::vec4 planes[6]) {
        // Left plane
        planes[0].x = vp[0][3] + vp[0][0];
        planes[0].y = vp[1][3] + vp[1][0];
        planes[0].z = vp[2][3] + vp[2][0];
        planes[0].w = vp[3][3] + vp[3][0];
        
        // Right plane  
        planes[1].x = vp[0][3] - vp[0][0];
        planes[1].y = vp[1][3] - vp[1][0];
        planes[1].z = vp[2][3] - vp[2][0];
        planes[1].w = vp[3][3] - vp[3][0];
        
        // Bottom plane
        planes[2].x = vp[0][3] + vp[0][1];
        planes[2].y = vp[1][3] + vp[1][1];
        planes[2].z = vp[2][3] + vp[2][1];
        planes[2].w = vp[3][3] + vp[3][1];
        
        // Top plane
        planes[3].x = vp[0][3] - vp[0][1];
        planes[3].y = vp[1][3] - vp[1][1];
        planes[3].z = vp[2][3] - vp[2][1];
        planes[3].w = vp[3][3] - vp[3][1];
        
        // Near plane
        planes[4].x = vp[0][3] + vp[0][2];
        planes[4].y = vp[1][3] + vp[1][2];
        planes[4].z = vp[2][3] + vp[2][2];
        planes[4].w = vp[3][3] + vp[3][2];
        
        // Far plane
        planes[5].x = vp[0][3] - vp[0][2];
        planes[5].y = vp[1][3] - vp[1][2];
        planes[5].z = vp[2][3] - vp[2][2];
        planes[5].w = vp[3][3] - vp[3][2];
        
        // Normalize the planes
        for (int i = 0; i < 6; i++) {
            float length = glm::length(glm::vec3(planes[i]));
            planes[i] /= length;
        }
    }
    
    void updateChunkDataBuffer() {
        if (!chunkManager) return;
        
        const auto& indirectManager = chunkManager->getIndirectDrawManager();
        const auto& chunkData = indirectManager.getChunkData();
        
        if (chunkData.empty()) return;
        
        // Update chunk data buffer
        int renderDistance = chunkManager->getRenderDistance();
        size_t maxChunks = calculateMaxChunks(renderDistance);
        size_t chunkCount = std::min(chunkData.size(), maxChunks);
        
        void* data;
        vkMapMemory(device, chunkDataBufferMemory, 0,
                   sizeof(Zerith::ChunkDrawData) * chunkCount, 0, &data);
        memcpy(data, chunkData.data(), sizeof(Zerith::ChunkDrawData) * chunkCount);
        vkUnmapMemory(device, chunkDataBufferMemory);
    }
    
    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.time = time;

        // Camera parameters for free-look camera
        glm::vec3 playerPos = player ? player->getPosition() + glm::vec3(0.0f, player->getEyeHeight(), 0.0f) : glm::vec3(0.0f);
        glm::vec3 playerRot = player ? player->getRotation() : glm::vec3(0.0f);
        

        // Create view matrix using lookAt
        // Calculate the forward direction from pitch and yaw
        glm::vec3 forward;
        forward.x = cos(playerRot.y) * cos(playerRot.x);
        forward.y = sin(playerRot.x);
        forward.z = sin(playerRot.y) * cos(playerRot.x);
        forward = glm::normalize(forward);
        
        glm::vec3 target = playerPos + forward;
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        ubo.view = glm::lookAt(playerPos, target, up);

        // Create projection matrix (Vulkan needs Y-flip)
        float aspect = swapChainExtent.width / (float)swapChainExtent.height;
        float fov = glm::radians(45.0f);
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        ubo.proj = glm::perspective(fov, aspect, nearPlane, farPlane);
        ubo.proj[1][1] *= -1; // Flip Y coordinate for Vulkan
        
        // Set face count for dynamic rendering
        ubo.faceCount = static_cast<uint32_t>(currentInstances.faces.size());
        
        // Set camera position
        ubo.cameraPos = glm::vec4(playerPos, 1.0f);
        
        // Extract frustum planes from view-projection matrix
        glm::mat4 viewProj = ubo.proj * ubo.view;
        extractFrustumPlanes(viewProj, ubo.frustumPlanes);
        
        
        // Update AABB debug data
        if (aabbDebugRenderer) {
            // Clear previous frame's AABBs
            aabbDebugRenderer->clear();
            
            // Always add the block outline for the block player is looking at
            if (player) {
                glm::ivec3 lookedAtBlock;
                if (player->getLookedAtBlock(lookedAtBlock)) {
                    // Create AABB for the looked-at block
                    Zerith::AABB blockAABB;
                    blockAABB.min = glm::vec3(lookedAtBlock);
                    blockAABB.max = blockAABB.min + glm::vec3(1.0f);
                    
                    // Add as a white highlighted block outline
                    aabbDebugRenderer->addAABB(blockAABB, glm::vec3(1.0f, 1.0f, 1.0f));
                }
            }
            
            // Add debug AABBs if debug mode is enabled
            if (showDebugAABBs) {
                // Add player AABB
                if (player) {
                    aabbDebugRenderer->addPlayerAABB(player->getAABB());
                }
                
                // Add block AABBs near player
                if (player && chunkManager) {
                    Zerith::AABB searchRegion = player->getAABB();
                    searchRegion.min -= glm::vec3(3.0f);
                    searchRegion.max += glm::vec3(3.0f);
                    auto blockAABBs = Zerith::CollisionSystem::getBlockAABBsInRegion(searchRegion, chunkManager.get());
                    aabbDebugRenderer->addBlockAABBs(blockAABBs);
                }
            }
            
            // Copy AABB data to buffer
            const auto& debugData = aabbDebugRenderer->getDebugData();
            if (!debugData.empty() && aabbInstanceBufferMapped) {
                int renderDistance = chunkManager ? chunkManager->getRenderDistance() : 8;
                size_t maxChunks = calculateMaxChunks(renderDistance);
                size_t copySize = std::min(debugData.size(), maxChunks) * sizeof(Zerith::AABBDebugData);
                memcpy(aabbInstanceBufferMapped, debugData.data(), copySize);
            }
            
            // Update AABB count in UBO (reuse faceCount field when in debug mode)
            if (aabbDebugRenderer->getCount() > 0) {
                ubo.faceCount = static_cast<uint32_t>(aabbDebugRenderer->getCount());
            }
        }

        // Copy UBO data to buffer
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
        
        // Update grass texture indices
        GrassTextureIndices grassIndices = getGrassTextureIndices();
        memcpy(grassTextureBuffersMapped[currentImage], &grassIndices, sizeof(grassIndices));
        
        // Update transparency texture indices
        TransparencyTextureIndices transparencyIndices = getTransparencyTextureIndices();
        memcpy(transparencyTextureBuffersMapped[currentImage], &transparencyIndices, sizeof(transparencyIndices));
        
        // Store for debug statistics
        lastUBO = ubo;
    }
    
    GrassTextureIndices getGrassTextureIndices() {
        GrassTextureIndices indices;
        
        // Get the texture array from the chunk mesh generator
        if (chunkManager) {
            auto meshGenerator = chunkManager->getMeshGenerator();
            if (meshGenerator) {
                auto textureArray = meshGenerator->getTextureArray();
                
                // Query texture layer indices for grass textures
                indices.grassTopLayer = textureArray->getTextureLayerByPath("assets/zerith/textures/block/grass_block_top.png");
                indices.grassSideLayer = textureArray->getTextureLayerByPath("assets/zerith/textures/block/grass_block_side.png");
                indices.grassOverlayLayer = textureArray->getTextureLayerByPath("assets/zerith/textures/block/grass_block_side_overlay.png");
                indices.padding = 0;
                
                return indices;
            }
        }
        
        // Fallback if chunk manager or mesh generator is not available
        indices.grassTopLayer = 0xFFFFFFFF; // MISSING_TEXTURE_LAYER
        indices.grassSideLayer = 0xFFFFFFFF;
        indices.grassOverlayLayer = 0xFFFFFFFF;
        indices.padding = 0;
        
        return indices;
    }
    
    TransparencyTextureIndices getTransparencyTextureIndices() {
        TransparencyTextureIndices indices;
        
        // Get the texture array from the chunk mesh generator
        if (chunkManager) {
            auto meshGenerator = chunkManager->getMeshGenerator();
            if (meshGenerator) {
                auto textureArray = meshGenerator->getTextureArray();
                
                // Query texture layer indices for transparency textures
                indices.waterLayer = textureArray->getTextureLayerByPath("assets/zerith/textures/block/water_still.png");
                indices.glassLayer = textureArray->getTextureLayerByPath("assets/zerith/textures/block/glass.png");
                indices.leavesLayer = textureArray->getTextureLayerByPath("assets/zerith/textures/block/oak_leaves.png");
                indices.padding = 0;
                
                return indices;
            }
        }
        
        // Fallback if chunk manager or mesh generator is not available
        indices.waterLayer = 0xFFFFFFFF; // MISSING_TEXTURE_LAYER
        indices.glassLayer = 0xFFFFFFFF;
        indices.leavesLayer = 0xFFFFFFFF;
        indices.padding = 0;
        
        return indices;
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
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
        clearValues[0].color = {0.05f, 0.05f, 0.05f, 1.0f}; // Slightly darker background
        clearValues[1].depthStencil = {1.0f, 0}; // Clear depth to 1.0 (far)
        
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Set up viewport and scissor once (shared across all pipelines)
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // For now, render all faces with opaque pipeline to verify basic rendering works
        // The layered separation is working, but we need to update descriptor sets properly
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaquePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
        
        // Set render layer push constant for opaque rendering
        uint32_t renderLayer = 0; // OPAQUE
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &renderLayer);

        // Helper lambda to render with current setup
        auto renderCurrentFaces = [&]() {
            if (chunkManager && currentInstances.faces.size() > 0) {
                const auto& indirectManager = chunkManager->getIndirectDrawManager();
                const auto& drawCommands = indirectManager.getDrawCommands();
                
                if (!drawCommands.empty()) {
                    updateIndirectDrawBuffer();
                    updateChunkDataBuffer();
                    vkCmdDrawMeshTasksIndirectEXT(commandBuffer, indirectDrawBuffer, 0, 1, 
                                                sizeof(Zerith::DrawMeshTasksIndirectCommand));
                }
            } else if (currentInstances.faces.size() > 0) {
                uint32_t facesPerWorkgroup = 32;
                uint32_t faceCount = static_cast<uint32_t>(currentInstances.faces.size());
                uint32_t totalWorkgroups = (faceCount + facesPerWorkgroup - 1) / facesPerWorkgroup;
                vkCmdDrawMeshTasksEXT(commandBuffer, totalWorkgroups, 1, 1);
            }
        };

        // OPAQUE PASS - Render all faces, shader will handle opaque materials
        renderCurrentFaces();

        // CUTOUT PASS - Render all faces again, shader will handle cutout materials (leaves)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cutoutPipeline);
        renderLayer = 1; // CUTOUT
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &renderLayer);
        renderCurrentFaces();

        // TRANSLUCENT PASS - Render all faces again, shader will handle translucent materials (water/glass)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, translucentPipeline);
        renderLayer = 2; // TRANSLUCENT
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &renderLayer);
        renderCurrentFaces();
        
        // Draw AABB debug wireframes (including block outline)
        if (aabbDebugRenderer && aabbDebugRenderer->getCount() > 0) {
            // Switch pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aabbDebugPipeline);
            
            // Bind AABB descriptor set
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  aabbPipelineLayout, 0, 1, &aabbDescriptorSets[imageIndex], 0, nullptr);
            
            // Draw AABBs using mesh shader
            uint32_t aabbCount = static_cast<uint32_t>(aabbDebugRenderer->getCount());
            vkCmdDrawMeshTasksEXT(commandBuffer, aabbCount, 1, 1);
        }

        // Render ImGui
        imguiIntegration.render(commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame() {
        // Start new ImGui frame
        imguiIntegration.newFrame();
        
        // Render consolidated debug window
        imguiIntegration.renderDebugWindow(player.get(), chunkManager.get());
        
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                             imageAvailableSemaphores[currentFrame % swapChainImages.size()], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(imageIndex);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[imageIndex], 0);
        recordCommandBuffer(commandBuffers[imageIndex], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame % swapChainImages.size()]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};
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

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanupSwapChain() {
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
            
            vkDestroyBuffer(device, grassTextureBuffers[i], nullptr);
            vkFreeMemory(device, grassTextureBuffersMemory[i], nullptr);
            
            vkDestroyBuffer(device, transparencyTextureBuffers[i], nullptr);
            vkFreeMemory(device, transparencyTextureBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        if (graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
        }
        if (opaquePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, opaquePipeline, nullptr);
        }
        if (cutoutPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, cutoutPipeline, nullptr);
        }
        if (translucentPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, translucentPipeline, nullptr);
        }
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createDepthResources();
        createRenderPass();
        createGraphicsPipeline();
        createLayeredRenderPipelines();
        createAABBDebugPipeline();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createAABBDescriptorSets();
    }

    void mainLoop() {
        // For calculating delta time
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        auto lastStatsTime = std::chrono::high_resolution_clock::now();
        
        while (!glfwWindowShouldClose(window)) {
            // Calculate delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;
            
            // Update ImGui performance metrics
            imguiIntegration.updatePerformanceMetrics(deltaTime);
            
            // Process input
            processInput(deltaTime);
            
            // Poll events and render
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }
    
    void processInput(float deltaTime) {
        if (!player) return;
        
        glm::vec3 oldPosition = player->getPosition();
        
        // Handle player input and physics (only when mouse is captured)
        if (mouseCaptured) {
            player->handleInput(window, deltaTime, chunkManager.get());
        }
        player->update(deltaTime, chunkManager.get());
        
        // Update chunks if player moved
        if (player->getPosition() != oldPosition) {
            updateChunks();
        }
    }
    

    void cleanup() {
        cleanupSwapChain();
        
        // Clean up texture resources
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);
        
        // Clean up storage buffer
        if (faceInstanceBufferMapped) {
            vkUnmapMemory(device, faceInstanceBufferMemory);
        }
        vkDestroyBuffer(device, faceInstanceBuffer, nullptr);
        vkFreeMemory(device, faceInstanceBufferMemory, nullptr);
        
        // Clean up layered instance buffers
        if (opaqueInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, opaqueInstanceBuffer, nullptr);
            vkFreeMemory(device, opaqueInstanceBufferMemory, nullptr);
        }
        if (cutoutInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, cutoutInstanceBuffer, nullptr);
            vkFreeMemory(device, cutoutInstanceBufferMemory, nullptr);
        }
        if (translucentInstanceBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, translucentInstanceBuffer, nullptr);
            vkFreeMemory(device, translucentInstanceBufferMemory, nullptr);
        }
        
        // Clean up AABB debug resources
        if (aabbInstanceBufferMapped) {
            vkUnmapMemory(device, aabbInstanceBufferMemory);
        }
        vkDestroyBuffer(device, aabbInstanceBuffer, nullptr);
        vkFreeMemory(device, aabbInstanceBufferMemory, nullptr);
        
        // Clean up indirect draw buffers
        vkDestroyBuffer(device, indirectDrawBuffer, nullptr);
        vkFreeMemory(device, indirectDrawBufferMemory, nullptr);
        vkDestroyBuffer(device, chunkDataBuffer, nullptr);
        vkFreeMemory(device, chunkDataBufferMemory, nullptr);
        
        vkDestroyPipeline(device, aabbDebugPipeline, nullptr);
        vkDestroyPipelineLayout(device, aabbPipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, aabbDescriptorSetLayout, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        // Clean up semaphores (one per swapchain image)
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        }
        
        // Clean up fences (one per frame in flight)
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        // Clean up ImGui before destroying device
        imguiIntegration.cleanup();

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

int main() {
    // Initialize logger
    Logger& logger = Logger::getInstance();
    #ifdef NDEBUG
        logger.setLogLevel(spdlog::level::info);  // Release mode: only INFO and above
    #else
        logger.setLogLevel(spdlog::level::debug); // Debug mode: all logs including DEBUG
    #endif
    logger.addLogFile("logs/meshshader.log");
    
    LOG_INFO("Zerith application starting...");
    
    // Initialize global thread pool
    Zerith::initializeThreadPool();
    
    ZerithApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        LOG_FATAL("Application crashed: %s", e.what());
        Zerith::shutdownThreadPool();
        return EXIT_FAILURE;
    }

    LOG_INFO("Zerith application shutting down gracefully");
    
    // Shutdown thread pool
    Zerith::shutdownThreadPool();
    
    logger.shutdown();
    return EXIT_SUCCESS;
}