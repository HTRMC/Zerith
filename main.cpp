#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <array>
#include <set>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <spng.h>  // libspng for PNG loading

// Face instance struct for cube faces
struct FaceInstance {
    glm::vec3 position;
    glm::quat rotation;
};

// Array of face instances in Vulkan coordinates (Y-up, right-handed)
const std::array<FaceInstance, 6> faceInstances = {
    // Top (Y+): Blender: Pos(0.5, 0.5, 1), Rot(0, 0, 90) - RED
    FaceInstance{
        glm::vec3(0.5f, 1.0f, -0.5f),
        glm::quat(glm::vec3(glm::radians(-90.0f), 0.0f, glm::radians(90.0f)))
    },
    
    // Bottom (Y-): Blender: Pos(0.5, 0.5, 0), Rot(180, 0, 90) - GREEN
    FaceInstance{
        glm::vec3(0.5f, 0.0f, -0.5f),
        glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, glm::radians(90.0f)))
    },
    
    // Front (Z+): Blender: Pos(0, 0.5, 0.5), Rot(-90, 180, 90) - BLUE
    FaceInstance{
        glm::vec3(0.0f, 0.5f, -0.5f),
        glm::quat(glm::vec3(glm::radians(0.0f), glm::radians(180.0f), 0.0f))
    },
    
    // Back (Z-): Blender: Pos(1, 0.5, 0.5), Rot(-90, 180, -90) - YELLOW
    FaceInstance{
        glm::vec3(1.0f, 0.5f, -0.5f),
        glm::quat(glm::vec3(glm::radians(0.0f), 0.0f, 0.0f))
    },
    
    // Left (X-): Blender: Pos(0.5, 0, 0.5), Rot(90, 0, 0) - MAGENTA
    FaceInstance{
        glm::vec3(0.5f, 0.5f, -0.0f),
        glm::quat(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f))
    },
    
    // Right (X+): Blender: Pos(0.5, 1, 0.5), Rot(-90, 180, 0) - CYAN
    FaceInstance{
        glm::vec3(0.5f, 0.5f, -1.0f),
        glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f))
    }
};

// Texture data structure
struct TextureData {
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    std::vector<uint8_t> pixels;
};

// Function to load PNG using libspng
TextureData loadPNG(const std::string& filename) {
    TextureData texture;

    // Open the PNG file
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
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
    std::cout << "PNG Info: " << filename << std::endl;
    std::cout << "  - Width: " << ihdr.width << std::endl;
    std::cout << "  - Height: " << ihdr.height << std::endl;
    std::cout << "  - Bit depth: " << (int)ihdr.bit_depth << std::endl;
    std::cout << "  - Color type: " << (int)ihdr.color_type << std::endl;
    
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

    // Decode the image
    ret = spng_decode_image(ctx, texture.pixels.data(), out_size, fmt, 0);
    if (ret) {
        spng_ctx_free(ctx);
        fclose(fp);
        throw std::runtime_error("Failed to decode image: " + std::string(spng_strerror(ret)));
    }
    
    // Verify image data (check first few pixels)
    if (!texture.pixels.empty()) {
        std::cout << "  - First 4 pixels (RGBA values):" << std::endl;
        for (int i = 0; i < 4 && i * 4 < texture.pixels.size(); i++) {
            std::cout << "    Pixel " << i << ": "
                      << (int)texture.pixels[i*4] << ", "
                      << (int)texture.pixels[i*4+1] << ", "
                      << (int)texture.pixels[i*4+2] << ", "
                      << (int)texture.pixels[i*4+3] << std::endl;
        }
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
        std::cerr << "Cannot save debug image: No valid pixel data" << std::endl;
        return;
    }
    
    // Open output file
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        std::cerr << "Failed to open file for writing debug image: " << filename << std::endl;
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
    std::cout << "Debug image saved to: " << filename << std::endl;
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
struct CompressedUBO {
    // Time value for animation (4 bytes)
    alignas(4) float time;

    // Camera data (8 bytes) - packed as 4 half-precision floats
    alignas(8) uint32_t packedCamera[2];

    // Projection data (8 bytes) - packed as 4 half-precision floats
    alignas(8) uint32_t packedProjection[2];

    // Total: 20 bytes (vs original 192 bytes)
};

uint32_t packHalf2(float a, float b) {
    return glm::packHalf2x16(glm::vec2(a, b));
}

uint32_t packUnorm2(float a, float b) {
    return glm::packUnorm2x16(glm::vec2(a, b));
}

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
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

// Main application class
class MeshShaderApplication {
public:
    void run() {
        initWindow();
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
    VkPipeline graphicsPipeline;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // Texture resources
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    // Mesh shader specific function pointers
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT;

    bool framebufferResized = false;

    size_t currentFrame = 0;
    
    // Camera parameters
    float cameraPitch = 0.0f;                      // Vertical angle in radians (-π/2 to π/2)
    float cameraYaw = 0.0f;                        // Horizontal angle in radians (start facing -Z)
    float cameraSpeed = 2.0f;                      // Units per second
    
    // Camera position in 3D space - positioned in front of cube
    glm::vec3 cameraPosition = glm::vec3(0.5f, 0.5f, 3.0f);
    
    // Camera orientation vectors - these will be updated in updateCameraVectors()
    glm::vec3 cameraFront;  // Direction camera is facing
    glm::vec3 cameraUp;     // Camera's up direction
    glm::vec3 cameraRight;  // Camera's right direction
    
    // Mouse input tracking
    bool firstMouse = true;
    float lastX = 0.0f, lastY = 0.0f;
    float mouseSensitivity = 0.1f;
    
    // Input state tracking
    bool keysPressed[348] = { false };  // GLFW supports up to KEY_LAST (348)

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Mesh Shader Face-Instanced Cube", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        
        // Set up callbacks
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        
        // Capture mouse cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // Initialize mouse position to center of the window
        lastX = WIDTH / 2.0f;
        lastY = HEIGHT / 2.0f;
        
        // Initialize camera vectors based on starting pitch/yaw
        updateCameraVectors();
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<MeshShaderApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
    
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto app = reinterpret_cast<MeshShaderApplication*>(glfwGetWindowUserPointer(window));
        
        // Only track keys we care about (prevents array access issues)
        if (key >= 0 && key < 348) {
            if (action == GLFW_PRESS) {
                app->keysPressed[key] = true;
            } else if (action == GLFW_RELEASE) {
                app->keysPressed[key] = false;
            }
        }
        
        // Allow escape key to exit
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
    }
    
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
        auto app = reinterpret_cast<MeshShaderApplication*>(glfwGetWindowUserPointer(window));
        
        if (app->firstMouse) {
            app->lastX = static_cast<float>(xpos);
            app->lastY = static_cast<float>(ypos);
            app->firstMouse = false;
        }
        
        // Calculate cursor movement
        float xoffset = static_cast<float>(xpos) - app->lastX;
        float yoffset = app->lastY - static_cast<float>(ypos); // Reversed so that moving up increases pitch
        
        app->lastX = static_cast<float>(xpos);
        app->lastY = static_cast<float>(ypos);
        
        // Apply sensitivity
        xoffset *= app->mouseSensitivity;
        yoffset *= app->mouseSensitivity;
        
        // Update camera angles
        app->cameraYaw += glm::radians(xoffset);
        app->cameraPitch += glm::radians(yoffset); // Not inverted anymore
        
        // Constrain pitch to avoid gimbal lock
        app->cameraPitch = glm::clamp(app->cameraPitch, -glm::radians(89.0f), glm::radians(89.0f));
        
        // Update camera direction vectors
        app->updateCameraVectors();
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createCommandPool();  // Create command pool earlier
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createGraphicsPipeline();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
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
        // Load PNG image, use default texture if loading fails
        TextureData textureData;
        
        try {
            // Try both paths - for running from source dir or from build dir
            try {
                textureData = loadPNG("assets/oak_planks.png");
                std::cout << "Loaded texture: assets/oak_planks.png" << std::endl;
            } catch (const std::runtime_error&) {
                textureData = loadPNG("../assets/oak_planks.png");
                std::cout << "Loaded texture: ../assets/oak_planks.png" << std::endl;
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Failed to load texture, using default checkerboard instead: " << e.what() << std::endl;
            textureData = createDefaultTexture();
        }
        
        VkDeviceSize imageSize = textureData.width * textureData.height * textureData.channels;
        
        // Create a staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        createBuffer(
            imageSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, 
            stagingBufferMemory
        );
        
        // Copy pixel data to staging buffer
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, textureData.pixels.data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        // Create texture image
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        if (textureData.channels == 3) {
            // If the PNG doesn't have an alpha channel, we need to convert from RGB to RGBA
            imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        }
        
        createImage(
            textureData.width, 
            textureData.height, 
            imageFormat, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            textureImage, 
            textureImageMemory
        );
        
        // Transition image layout for copy operation
        transitionImageLayout(
            textureImage, 
            imageFormat, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );
        
        // Copy from staging buffer to texture image
        copyBufferToImage(
            stagingBuffer, 
            textureImage, 
            textureData.width, 
            textureData.height
        );
        
        // Transition image layout for shader access
        transitionImageLayout(
            textureImage, 
            imageFormat, 
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
    
    void createTextureImageView() {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
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
        std::cout << "Device features:" << std::endl;
        std::cout << "  - Mesh shader: " << (meshShaderFeatures.meshShader ? "supported" : "not supported") << std::endl;
        std::cout << "  - Task shader: " << (meshShaderFeatures.taskShader ? "supported" : "not supported") << std::endl;
        std::cout << "  - Maintenance4: " << (maintenance4Features.maintenance4 ? "supported" : "not supported") << std::endl;

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
            std::cout << "Anisotropic filtering enabled" << std::endl;
        } else {
            std::cout << "Anisotropic filtering not available" << std::endl;
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
            std::cout << "Maintenance4 feature enabled" << std::endl;
        } else {
            maintenance4Features.maintenance4 = VK_FALSE;
            std::cout << "Warning: Maintenance4 feature not available, shader may not work properly" << std::endl;
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

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
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

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

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
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

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
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

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

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
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
        VkDeviceSize bufferSize = sizeof(CompressedUBO);

        uniformBuffers.resize(swapChainImages.size());
        uniformBuffersMemory.resize(swapChainImages.size());
        uniformBuffersMapped.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
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
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

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
            bufferInfo.range = sizeof(CompressedUBO);
            
            // Image sampler info
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            
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

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        CompressedUBO compressedUbo{};
        compressedUbo.time = time;  // Send the time to the shader for animation

        // Camera parameters for free-look camera
        // First value is now X position
        float camPosX = cameraPosition.x;
        float camPitch = cameraPitch;
        
        // Second pair is Y position and Yaw
        float camPosY = cameraPosition.y;
        float camYaw = cameraYaw;
        
        // Third pair (using time field temporarily) is Z position
        float camPosZ = cameraPosition.z;

        // Projection parameters
        float aspect = swapChainExtent.width / (float)swapChainExtent.height;
        float fov = 0.785f; // 45 degrees in radians

        // Pack camera position, orientation and projection data
        compressedUbo.packedCamera[0] = packHalf2(camPosX, camPitch);
        compressedUbo.packedCamera[1] = packHalf2(camPosY, camYaw);
        compressedUbo.packedProjection[0] = packHalf2(fov, aspect);
        compressedUbo.packedProjection[1] = packHalf2(camPosZ, 10.0f); // Use near value to store Z position

        // Copy compressed data to buffer
        memcpy(uniformBuffersMapped[currentImage], &compressedUbo, sizeof(compressedUbo));
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

        VkClearValue clearColor = {{{0.05f, 0.05f, 0.05f, 1.0f}}}; // Slightly darker background
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);

        // Draw the cube using a single task shader workgroup
        // The mesh shader will create all 6 faces from a single quad
        vkCmdDrawMeshTasksEXT(commandBuffer, 1, 1, 1);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                                             imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

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
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
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
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
    }

    void mainLoop() {
        // For calculating delta time
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        
        while (!glfwWindowShouldClose(window)) {
            // Calculate delta time
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;
            
            // Process input
            processInput(deltaTime);
            
            // Poll events and render
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }
    
    void processInput(float deltaTime) {
        float velocity = cameraSpeed * deltaTime;
        
        // Forward/backward movement (W/S)
        if (keysPressed[GLFW_KEY_W]) {
            cameraPosition += cameraFront * velocity;
        }
        if (keysPressed[GLFW_KEY_S]) {
            cameraPosition -= cameraFront * velocity;
        }
        
        // Strafe left/right movement (A/D)
        if (keysPressed[GLFW_KEY_A]) {
            cameraPosition -= cameraRight * velocity;
        }
        if (keysPressed[GLFW_KEY_D]) {
            cameraPosition += cameraRight * velocity;
        }
        
        // Optional: Up/down movement (Space/Ctrl)
        if (keysPressed[GLFW_KEY_SPACE]) {
            cameraPosition += cameraUp * velocity;
        }
        if (keysPressed[GLFW_KEY_LEFT_CONTROL]) {
            cameraPosition -= cameraUp * velocity;
        }
    }
    
    void updateCameraVectors() {
        // Calculate the new front vector using standard OpenGL/Vulkan convention
        // - Yaw 0 = looking along negative Z
        // - Yaw π/2 = looking along positive X
        // - Pitch 0 = looking horizontally
        // - Positive pitch = looking up
        glm::vec3 direction;
        direction.x = sin(cameraYaw) * cos(cameraPitch);
        direction.y = sin(cameraPitch);
        direction.z = -cos(cameraYaw) * cos(cameraPitch);
        
        cameraFront = glm::normalize(direction);
        
        // Use a fixed world up vector for stability
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // Recalculate the right vector
        cameraRight = glm::normalize(glm::cross(cameraFront, worldUp));
        
        // Recalculate the up vector
        // This ensures the up vector is orthogonal to both front and right
        cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));
    }

    void cleanup() {
        cleanupSwapChain();
        
        // Clean up texture resources
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

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
    MeshShaderApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}