// main.cpp
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <optional>
#include <set>
#include <fstream>
#include <array>
#include <chrono>
#define STB_IMAGE_IMPLEMENTATION
#if defined(_WIN32) || defined(_WIN64)
#include <stb_image.h>
#else
    #include <stb/stb_image.h>
#endif

struct Block {
    bool exists;
    glm::vec3 position;
};

// Add to private members
std::vector<Block> blocks;
const float MAX_REACH = 5.0f; // Maximum distance for block interaction

struct PlayerCollider {
    glm::vec3 position; // Center of the player collider
    glm::vec3 dimensions = glm::vec3(0.6f, 1.8f, 0.6f); // Width, height, depth
    bool onGround = false;
    glm::vec3 velocity = glm::vec3(0.0f);
    float gravity = -20.0f;
    float jumpForce = 8.0f;
    float walkingSpeed = 4.317f; // Blocks per second
    float sprintingSpeed = 5.612f; // Blocks per second while sprinting
    float sprintJumpForce = 9.5f; // Increased jump force while sprinting

    // Sprinting state
    bool isSprinting = false;
    double lastWPress = 0.0;
    bool wasWPressed = false;
    static constexpr double DOUBLE_TAP_TIME = 0.3; // Time window for double-tap detection in seconds
};

PlayerCollider player;

struct Vertex {
    float pos[3];
    float color[3];
    float texCoord[2];

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

struct SelectedBlock {
    bool hasSelection;
    glm::vec3 position;
} selectedBlock;

// Add these new vertex data structures after the existing Block struct
struct LineVertex {
    float pos[3];

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(LineVertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(LineVertex, pos);
        return attributeDescriptions;
    }
};

class Zerith {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window = nullptr;
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue graphicsQueue;
    vk::Queue presentQueue;
    vk::SurfaceKHR surface;
    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::ImageView> swapChainImageViews;
    vk::RenderPass renderPass;
    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;
    std::vector<vk::Framebuffer> swapChainFramebuffers;
    vk::CommandPool commandPool;
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::Buffer> uniformBuffers;
    std::vector<vk::DeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSetLayout descriptorSetLayout;
    vk::DescriptorSet descriptorSet;
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    size_t currentFrame = 0;
    bool framebufferResized = false;
    const int MAX_FRAMES_IN_FLIGHT = 2;
    bool windowFocused = false;
    bool cursorLocked = true;
    vk::Pipeline linesPipeline;
    vk::Buffer selectionBuffer;
    vk::DeviceMemory selectionBufferMemory;
    std::vector<LineVertex> selectionVertices;
    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    vk::Image textureImage;
    vk::DeviceMemory textureImageMemory;
    vk::ImageView textureImageView;
    vk::Sampler textureSampler;
    std::vector<vk::DescriptorSet> descriptorSets;

    vk::Format findDepthFormat() {
        std::vector<vk::Format> candidates = {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint
        };

        for (vk::Format format: candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);
            if ((props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
                vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
                return format;
            }
        }
        throw std::runtime_error("failed to find supported depth format!");
    }

    void createDepthResources() {
        vk::Format depthFormat = findDepthFormat();

        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = swapChainExtent.width;
        imageInfo.extent.height = swapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;

        depthImage = device.createImage(imageInfo);

        auto memRequirements = device.getImageMemoryRequirements(depthImage);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eDeviceLocal);

        depthImageMemory = device.allocateMemory(allocInfo);
        device.bindImageMemory(depthImage, depthImageMemory, 0);

        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = depthImage;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        depthImageView = device.createImageView(viewInfo);
    }

    void updateSelectionBuffer() {
        if (!selectedBlock.hasSelection) {
            // Clear the selection buffer
            selectionVertices.clear();
            void *data = device.mapMemory(selectionBufferMemory, 0, sizeof(LineVertex) * 24);
            memset(data, 0, sizeof(LineVertex) * 24);
            device.unmapMemory(selectionBufferMemory);
            return;
        }

        // Define the vertices for the block outline
        glm::vec3 pos = selectedBlock.position;
        float expand = 0.002f;
        selectionVertices = {
            // Front face
            {{pos.x - expand, pos.y - expand, pos.z - expand}},
            {{pos.x + 1.0f + expand, pos.y - expand, pos.z - expand}},

            {{pos.x + 1.0f + expand, pos.y - expand, pos.z - expand}},
            {{pos.x + 1.0f + expand, pos.y + 1.0f + expand, pos.z - expand}},

            {{pos.x + 1.0f + expand, pos.y + 1.0f + expand, pos.z - expand}},
            {{pos.x - expand, pos.y + 1.0f + expand, pos.z - expand}},

            {{pos.x - expand, pos.y + 1.0f + expand, pos.z - expand}},
            {{pos.x - expand, pos.y - expand, pos.z - expand}},

            // Back face
            {{pos.x - expand, pos.y - expand, pos.z + 1.0f + expand}},
            {{pos.x + 1.0f + expand, pos.y - expand, pos.z + 1.0f + expand}},

            {{pos.x + 1.0f + expand, pos.y - expand, pos.z + 1.0f + expand}},
            {{pos.x + 1.0f + expand, pos.y + 1.0f + expand, pos.z + 1.0f + expand}},

            {{pos.x + 1.0f + expand, pos.y + 1.0f + expand, pos.z + 1.0f + expand}},
            {{pos.x - expand, pos.y + 1.0f + expand, pos.z + 1.0f + expand}},

            {{pos.x - expand, pos.y + 1.0f + expand, pos.z + 1.0f + expand}},
            {{pos.x - expand, pos.y - expand, pos.z + 1.0f + expand}},

            // Connecting lines
            {{pos.x - expand, pos.y - expand, pos.z - expand}},
            {{pos.x - expand, pos.y - expand, pos.z + 1.0f + expand}},

            {{pos.x + 1.0f + expand, pos.y - expand, pos.z - expand}},
            {{pos.x + 1.0f + expand, pos.y - expand, pos.z + 1.0f + expand}},

            {{pos.x + 1.0f + expand, pos.y + 1.0f + expand, pos.z - expand}},
            {{pos.x + 1.0f + expand, pos.y + 1.0f + expand, pos.z + 1.0f + expand}},

            {{pos.x - expand, pos.y + 1.0f + expand, pos.z - expand}},
            {{pos.x - expand, pos.y + 1.0f + expand, pos.z + 1.0f + expand}}
        };

        // Update buffer data
        void *data = device.mapMemory(selectionBufferMemory, 0, sizeof(LineVertex) * selectionVertices.size());
        memcpy(data, selectionVertices.data(), sizeof(LineVertex) * selectionVertices.size());
        device.unmapMemory(selectionBufferMemory);

        createCommandBuffers();
    }

    void createSelectionBuffer() {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = sizeof(LineVertex) * 24; // 24 vertices for the complete outline
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        selectionBuffer = device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(selectionBuffer);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                                   vk::MemoryPropertyFlagBits::eHostCoherent);

        selectionBufferMemory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(selectionBuffer, selectionBufferMemory, 0);
    }


    void toggleCursorLock() {
        cursorLocked = !cursorLocked;
        if (cursorLocked) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            glfwSetCursorPos(window, width / 2.0, height / 2.0);
            camera.firstMouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    static void focusCallback(GLFWwindow *window, int focused) {
        auto app = reinterpret_cast<Zerith *>(glfwGetWindowUserPointer(window));
        app->windowFocused = focused == GLFW_TRUE;

        if (app->windowFocused && app->cursorLocked) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            glfwSetCursorPos(window, width / 2.0, height / 2.0);
        }
    }

    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;

    void initializeBlocks() {
        player.position = glm::vec3(8.0f, 20.0f, 8.0f);
        const int GRID_SIZE = 16;
        blocks.reserve(GRID_SIZE * GRID_SIZE * GRID_SIZE);

        for (int x = 0; x < GRID_SIZE; ++x) {
            for (int y = 0; y < GRID_SIZE; ++y) {
                for (int z = 0; z < GRID_SIZE; ++z) {
                    Block block;
                    block.exists = true;
                    block.position = glm::vec3(x, y, z);
                    blocks.push_back(block);
                }
            }
        }
        updateBlockMesh();
    }

    vk::CommandBuffer beginSingleTimeCommands() {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(beginInfo);
        return commandBuffer;
    }

    void endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
        commandBuffer.end();

        vk::SubmitInfo submitInfo{};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        graphicsQueue.submit(submitInfo);
        graphicsQueue.waitIdle();

        device.freeCommandBuffers(commandPool, 1, &commandBuffer);
    }

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::Buffer &buffer, vk::DeviceMemory &bufferMemory) {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        buffer = device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        bufferMemory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(buffer, bufferMemory, 0);
    }

    void createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image &image,
                     vk::DeviceMemory &imageMemory) {
        vk::ImageCreateInfo imageInfo{};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = vk::ImageLayout::eUndefined;
        imageInfo.usage = usage;
        imageInfo.sharingMode = vk::SharingMode::eExclusive;
        imageInfo.samples = vk::SampleCountFlagBits::e1;

        image = device.createImage(imageInfo);

        vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        imageMemory = device.allocateMemory(allocInfo);
        device.bindImageMemory(image, imageMemory, 0);
    }

    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout) {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::ImageMemoryBarrier barrier{};
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destinationStage;

        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
        vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

        vk::BufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = vk::Offset3D{0, 0, 0};
        region.imageExtent = vk::Extent3D{width, height, 1};

        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

        endSingleTimeCommands(commandBuffer);
    }

    void createTextureImage(const char *filepath) {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(filepath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        vk::Buffer stagingBuffer;
        vk::DeviceMemory stagingBufferMemory;

        createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);

        void *data = device.mapMemory(stagingBufferMemory, 0, imageSize);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        device.unmapMemory(stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

        transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(stagingBuffer, textureImage,
                          static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb,
                              vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

        device.destroyBuffer(stagingBuffer);
        device.freeMemory(stagingBufferMemory);
    }

    vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image = image;
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        return device.createImageView(viewInfo);
    }

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
    }

    void createTextureSampler() {
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter = vk::Filter::eNearest;
        samplerInfo.minFilter = vk::Filter::eNearest;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.anisotropyEnable = VK_FALSE;

        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = vk::CompareOp::eAlways;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;

        textureSampler = device.createSampler(samplerInfo);
    }

    void updateBlockMesh() {
        vertices.clear();
        indices.clear();

        for (const auto &block: blocks) {
            if (!block.exists) continue;

            // Get neighboring block positions
            glm::vec3 pos = block.position;
            bool hasNeighbor[6] = {false}; // left, right, bottom, top, front, back

            // Check for neighbors
            for (const auto &other: blocks) {
                if (!other.exists) continue;
                if (other.position == pos) continue;

                glm::vec3 diff = other.position - pos;
                if (glm::length(diff) > 1.01f) continue;

                // Check exact positions for neighbors
                if (diff.x == -1.0f && diff.y == 0.0f && diff.z == 0.0f) hasNeighbor[0] = true; // left
                else if (diff.x == 1.0f && diff.y == 0.0f && diff.z == 0.0f) hasNeighbor[1] = true; // right
                if (diff.y == -1.0f && diff.x == 0.0f && diff.z == 0.0f) hasNeighbor[2] = true; // bottom
                else if (diff.y == 1.0f && diff.x == 0.0f && diff.z == 0.0f) hasNeighbor[3] = true; // top
                if (diff.z == -1.0f && diff.x == 0.0f && diff.y == 0.0f) hasNeighbor[4] = true; // front
                else if (diff.z == 1.0f && diff.x == 0.0f && diff.y == 0.0f) hasNeighbor[5] = true; // back
            }

            uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

            // Front face (-Z)
            if (!hasNeighbor[4]) {
                vertices.push_back({{pos.x, pos.y, pos.z}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}); // Bottom-left
                vertices.push_back({{pos.x + 1, pos.y, pos.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}); // Bottom-right
                vertices.push_back({{pos.x + 1, pos.y + 1, pos.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}); // Top-right
                vertices.push_back({{pos.x, pos.y + 1, pos.z}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}); // Top-left

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);
                baseIndex += 4;
            }

            // Back face (+Z)
            if (!hasNeighbor[5]) {
                vertices.push_back({{pos.x + 1, pos.y, pos.z + 1}, {0.9f, 0.9f, 0.9f}, {0.0f, 1.0f}}); // Bottom-left
                vertices.push_back({{pos.x, pos.y, pos.z + 1}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f}}); // Bottom-right
                vertices.push_back({{pos.x, pos.y + 1, pos.z + 1}, {0.9f, 0.9f, 0.9f}, {1.0f, 0.0f}}); // Top-right
                vertices.push_back({{pos.x + 1, pos.y + 1, pos.z + 1}, {0.9f, 0.9f, 0.9f}, {0.0f, 0.0f}}); // Top-left

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);
                baseIndex += 4;
            }

            // Right face (+X)
            if (!hasNeighbor[1]) {
                vertices.push_back({{pos.x + 1, pos.y, pos.z}, {0.8f, 0.8f, 0.8f}, {0.0f, 1.0f}}); // Bottom-left
                vertices.push_back({{pos.x + 1, pos.y, pos.z + 1}, {0.8f, 0.8f, 0.8f}, {1.0f, 1.0f}}); // Bottom-right
                vertices.push_back({{pos.x + 1, pos.y + 1, pos.z + 1}, {0.8f, 0.8f, 0.8f}, {1.0f, 0.0f}}); // Top-right
                vertices.push_back({{pos.x + 1, pos.y + 1, pos.z}, {0.8f, 0.8f, 0.8f}, {0.0f, 0.0f}}); // Top-left

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);
                baseIndex += 4;
            }

            // Left face (-X)
            if (!hasNeighbor[0]) {
                vertices.push_back({{pos.x, pos.y, pos.z + 1}, {0.8f, 0.8f, 0.8f}, {0.0f, 1.0f}}); // Bottom-left
                vertices.push_back({{pos.x, pos.y, pos.z}, {0.8f, 0.8f, 0.8f}, {1.0f, 1.0f}}); // Bottom-right
                vertices.push_back({{pos.x, pos.y + 1, pos.z}, {0.8f, 0.8f, 0.8f}, {1.0f, 0.0f}}); // Top-right
                vertices.push_back({{pos.x, pos.y + 1, pos.z + 1}, {0.8f, 0.8f, 0.8f}, {0.0f, 0.0f}}); // Top-left

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);
                baseIndex += 4;
            }

            // Top face (+Y)
            if (!hasNeighbor[3]) {
                vertices.push_back({{pos.x, pos.y + 1, pos.z}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}); // Bottom-left
                vertices.push_back({{pos.x + 1, pos.y + 1, pos.z}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}); // Bottom-right
                vertices.push_back({{pos.x + 1, pos.y + 1, pos.z + 1}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}); // Top-right
                vertices.push_back({{pos.x, pos.y + 1, pos.z + 1}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}); // Top-left

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);
                baseIndex += 4;
            }

            // Bottom face (-Y)
            if (!hasNeighbor[2]) {
                vertices.push_back({{pos.x, pos.y, pos.z + 1}, {0.7f, 0.7f, 0.7f}, {0.0f, 1.0f}}); // Bottom-left
                vertices.push_back({{pos.x + 1, pos.y, pos.z + 1}, {0.7f, 0.7f, 0.7f}, {1.0f, 1.0f}}); // Bottom-right
                vertices.push_back({{pos.x + 1, pos.y, pos.z}, {0.7f, 0.7f, 0.7f}, {1.0f, 0.0f}}); // Top-right
                vertices.push_back({{pos.x, pos.y, pos.z}, {0.7f, 0.7f, 0.7f}, {0.0f, 0.0f}}); // Top-left

                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 2);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);
            }
        }

        // Recreate vertex and index buffers
        recreateBlockBuffers();
    }

    void recreateBlockBuffers() {
        // Create new buffers first
        vk::Buffer newVertexBuffer;
        vk::DeviceMemory newVertexBufferMemory;
        vk::Buffer newIndexBuffer;
        vk::DeviceMemory newIndexBufferMemory;

        // Create temporary buffers
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        newVertexBuffer = device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(newVertexBuffer);
        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                                   vk::MemoryPropertyFlagBits::eHostCoherent);

        newVertexBufferMemory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(newVertexBuffer, newVertexBufferMemory, 0);

        void *data = device.mapMemory(newVertexBufferMemory, 0, bufferInfo.size);
        memcpy(data, vertices.data(), (size_t) bufferInfo.size);
        device.unmapMemory(newVertexBufferMemory);

        // Create new index buffer
        bufferInfo.size = sizeof(indices[0]) * indices.size();
        bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;

        newIndexBuffer = device.createBuffer(bufferInfo);

        memRequirements = device.getBufferMemoryRequirements(newIndexBuffer);
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                                   vk::MemoryPropertyFlagBits::eHostCoherent);

        newIndexBufferMemory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(newIndexBuffer, newIndexBufferMemory, 0);

        data = device.mapMemory(newIndexBufferMemory, 0, bufferInfo.size);
        memcpy(data, indices.data(), (size_t) bufferInfo.size);
        device.unmapMemory(newIndexBufferMemory);

        // Wait for the device to finish any ongoing operations
        device.waitIdle();

        // Clean up old buffers
        if (vertexBuffer) {
            device.destroyBuffer(vertexBuffer);
            device.freeMemory(vertexBufferMemory);
        }
        if (indexBuffer) {
            device.destroyBuffer(indexBuffer);
            device.freeMemory(indexBufferMemory);
        }

        // Assign new buffers
        vertexBuffer = newVertexBuffer;
        vertexBufferMemory = newVertexBufferMemory;
        indexBuffer = newIndexBuffer;
        indexBufferMemory = newIndexBufferMemory;

        // Recreate command buffers to use new vertex/index buffers
        createCommandBuffers();
    }

    bool raycastBlock(glm::vec3 &hitPosition, glm::vec3 &hitNormal, bool &hit) {
        glm::vec3 rayStart = camera.pos;
        glm::vec3 rayDir = glm::normalize(camera.front);

        float closest = MAX_REACH;
        hit = false;
        bool selectionChanged = false;

        for (const auto &block: blocks) {
            if (!block.exists) continue;

            glm::vec3 mins = block.position;
            glm::vec3 maxs = block.position + glm::vec3(1.0f);

            float tmin = (mins.x - rayStart.x) / rayDir.x;
            float tmax = (maxs.x - rayStart.x) / rayDir.x;

            if (tmin > tmax) std::swap(tmin, tmax);

            float tymin = (mins.y - rayStart.y) / rayDir.y;
            float tymax = (maxs.y - rayStart.y) / rayDir.y;

            if (tymin > tymax) std::swap(tymin, tymax);

            if ((tmin > tymax) || (tymin > tmax)) continue;

            tmin = std::max(tmin, tymin);
            tmax = std::min(tmax, tymax);

            float tzmin = (mins.z - rayStart.z) / rayDir.z;
            float tzmax = (maxs.z - rayStart.z) / rayDir.z;

            if (tzmin > tzmax) std::swap(tzmin, tzmax);

            if ((tmin > tzmax) || (tzmin > tmax)) continue;

            tmin = std::max(tmin, tzmin);
            tmax = std::min(tmax, tzmax);

            if (tmin < 0) {
                if (tmax < 0) continue;
                tmin = tmax;
            }

            if (tmin < closest) {
                closest = tmin;
                hitPosition = rayStart + rayDir * tmin;

                glm::vec3 center = block.position + glm::vec3(0.5f);
                glm::vec3 diff = hitPosition - center;
                float x = abs(diff.x);
                float y = abs(diff.y);
                float z = abs(diff.z);

                if (x > y && x > z)
                    hitNormal = glm::vec3(diff.x > 0 ? 1 : -1, 0, 0);
                else if (y > z)
                    hitNormal = glm::vec3(0, diff.y > 0 ? 1 : -1, 0);
                else
                    hitNormal = glm::vec3(0, 0, diff.z > 0 ? 1 : -1);

                hit = true;
                if (!selectedBlock.hasSelection || selectedBlock.position != block.position) {
                    selectedBlock.hasSelection = true;
                    selectedBlock.position = block.position;
                    selectionChanged = true;
                }
            }
        }

        if (!hit && selectedBlock.hasSelection) {
            selectedBlock.hasSelection = false;
            selectionChanged = true;
        }

        if (selectionChanged) {
            updateSelectionBuffer();
        }

        return hit;
    }

    void handleBlockInteraction() {
        static bool leftPressed = false;
        static bool rightPressed = false;

        bool leftClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool rightClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        if (!cursorLocked) return;

        glm::vec3 hitPos, hitNormal;
        bool hit;

        if (raycastBlock(hitPos, hitNormal, hit)) {
            // Break block
            if (leftClick && !leftPressed) {
                for (auto &block: blocks) {
                    if (!block.exists) continue;

                    // Check if the hit position is within the block's bounds
                    glm::vec3 blockMin = block.position;
                    glm::vec3 blockMax = block.position + glm::vec3(1.0f);

                    if (hitPos.x >= blockMin.x && hitPos.x <= blockMax.x &&
                        hitPos.y >= blockMin.y && hitPos.y <= blockMax.y &&
                        hitPos.z >= blockMin.z && hitPos.z <= blockMax.z) {
                        block.exists = false;
                        updateBlockMesh();
                        break;
                    }
                }
            }
            // Place block
            else if (rightClick && !rightPressed) {
                glm::vec3 newPos = glm::floor(hitPos + hitNormal + glm::vec3(0.001f));
                // Add small offset to avoid floating point issues

                // Check if position is occupied
                bool occupied = false;
                for (const auto &block: blocks) {
                    if (!block.exists) continue;
                    if (block.position == newPos) {
                        occupied = true;
                        break;
                    }
                }

                if (!occupied) {
                    Block newBlock;
                    newBlock.exists = true;
                    newBlock.position = newPos;
                    blocks.push_back(newBlock);
                    updateBlockMesh();
                }
            }
        }

        leftPressed = leftClick;
        rightPressed = rightClick;
    }

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct CameraData {
        glm::vec3 pos = glm::vec3(8.0f, 18.0f, 8.0f);
        glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        float yaw = -90.0f;
        float pitch = 0.0f;
        float lastX = 400.0f;
        float lastY = 300.0f;
        bool firstMouse = true;
        float sensitivity = 0.1f;
        float speed = 2.5f;
    } camera;

    static void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
        auto app = reinterpret_cast<Zerith *>(glfwGetWindowUserPointer(window));
        app->processMouseMovement(xpos, ypos);
    }

    void processMouseMovement(double xposIn, double yposIn) {
        if (!windowFocused) return;

        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (camera.firstMouse) {
            camera.lastX = xpos;
            camera.lastY = ypos;
            camera.firstMouse = false;
            return;
        }

        float xoffset = xpos - camera.lastX;
        float yoffset = camera.lastY - ypos;
        camera.lastX = xpos;
        camera.lastY = ypos;

        xoffset *= camera.sensitivity;
        yoffset *= camera.sensitivity;

        camera.yaw += xoffset;
        camera.pitch += yoffset;

        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
            camera.pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        front.y = sin(glm::radians(camera.pitch));
        front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.front = glm::normalize(front);
    }

    void processInput() {
        static bool escapePressed = false;
        static bool spacePressed = false;
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            if (!escapePressed) {
                toggleCursorLock();
                escapePressed = true;
            }
        } else {
            escapePressed = false;
        }

        if (!cursorLocked) return;

        // Handle sprint input (Ctrl key or double-tap W)
        bool wKeyPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool ctrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                           glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

        // Double-tap W detection
        if (wKeyPressed && !player.wasWPressed) {
            double currentTime = glfwGetTime();
            double timeSinceLastPress = currentTime - player.lastWPress;

            if (timeSinceLastPress < player.DOUBLE_TAP_TIME) {
                player.isSprinting = true;
            }

            player.lastWPress = currentTime;
        }
        player.wasWPressed = wKeyPressed;

        // Direct sprint key (Ctrl) handling
        if (ctrlPressed && wKeyPressed) {
            player.isSprinting = true;
        }

        // Stop sprinting if not moving forward
        if (!wKeyPressed) {
            player.isSprinting = false;
        }

        // Calculate movement vector from input
        glm::vec3 movement(0.0f);
        if (wKeyPressed)
            movement += camera.front;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            movement -= camera.front;
            player.isSprinting = false; // Can't sprint backwards
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            movement -= glm::normalize(glm::cross(camera.front, camera.up));
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            movement += glm::normalize(glm::cross(camera.front, camera.up));

        // Remove vertical component for ground movement
        movement.y = 0;
        if (glm::length(movement) > 0) {
            movement = glm::normalize(movement);
        }

        // Handle jumping with sprint jump boost
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            if (!spacePressed && player.onGround) {
                // Apply sprint jump boost if sprinting
                player.velocity.y = player.isSprinting ? player.sprintJumpForce : player.jumpForce;
                player.onGround = false;
            }
            spacePressed = true;
        } else {
            spacePressed = false;
        }

        // Apply movement speed based on sprint state
        float currentSpeed = player.isSprinting ? player.sprintingSpeed : player.walkingSpeed;
        float speed = currentSpeed * deltaTime;

        // Apply horizontal movement
        player.velocity.x = movement.x * speed * 100.0f;
        player.velocity.z = movement.z * speed * 100.0f;

        // Apply gravity
        if (!player.onGround) {
            player.velocity.y += player.gravity * deltaTime;
        }

        // Calculate new position
        glm::vec3 newPos = player.position + player.velocity * deltaTime;

        // Resolve collisions
        resolveCollisions(newPos);

        // Update player and camera positions
        player.position = newPos;
        camera.pos = player.position + glm::vec3(0.0f, player.dimensions.y / 2, 0.0f);

        updateUniformBuffer(currentFrame);
    }

    // Add a helper function to reset sprint state when needed
    void resetSprintState() {
        player.isSprinting = false;
        player.wasWPressed = false;
        player.lastWPress = 0.0;
    }


    bool checkCollision(const glm::vec3 &pos, const glm::vec3 &dimensions, const Block &block) {
        if (!block.exists) return false;

        // AABB collision check
        bool collisionX = pos.x + dimensions.x / 2 > block.position.x &&
                          block.position.x + 1.0f > pos.x - dimensions.x / 2;
        bool collisionY = pos.y + dimensions.y / 2 > block.position.y &&
                          block.position.y + 1.0f > pos.y - dimensions.y / 2;
        bool collisionZ = pos.z + dimensions.z / 2 > block.position.z &&
                          block.position.z + 1.0f > pos.z - dimensions.z / 2;

        return collisionX && collisionY && collisionZ;
    }

    void resolveCollisions(glm::vec3 &newPos) {
        // Check collisions for each axis independently
        glm::vec3 testPos = player.position;

        // X axis
        testPos.x = newPos.x;
        for (const auto &block: blocks) {
            if (checkCollision(testPos, player.dimensions, block)) {
                newPos.x = player.position.x;
                player.velocity.x = 0;
                break;
            }
        }

        // Y axis
        testPos = player.position;
        testPos.y = newPos.y;
        player.onGround = false;
        for (const auto &block: blocks) {
            if (checkCollision(testPos, player.dimensions, block)) {
                if (newPos.y < player.position.y) {
                    player.onGround = true;
                }
                newPos.y = player.position.y;
                player.velocity.y = 0;
                break;
            }
        }

        // Z axis
        testPos = player.position;
        testPos.z = newPos.z;
        for (const auto &block: blocks) {
            if (checkCollision(testPos, player.dimensions, block)) {
                newPos.z = player.position.z;
                player.velocity.z = 0;
                break;
            }
        }
    }

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto app = reinterpret_cast<Zerith *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initWindow() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

        window = glfwCreateWindow(800, 600, "Zerith", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetWindowFocusCallback(window, focusCallback);

        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            int monitorX, monitorY;
            glfwGetMonitorPos(monitor, &monitorX, &monitorY);
            glfwSetWindowPos(window,
                             monitorX + (mode->width - 800) / 2,
                             monitorY + (mode->height - 600) / 2);
        }
    }

    void cleanupSwapChain() {
        device.destroyImageView(depthImageView);
        device.destroyImage(depthImage);
        device.freeMemory(depthImageMemory);

        for (auto framebuffer: swapChainFramebuffers) {
            device.destroyFramebuffer(framebuffer);
        }

        device.freeCommandBuffers(commandPool, commandBuffers);
        device.destroyPipeline(graphicsPipeline);
        device.destroyPipeline(linesPipeline); // Don't forget to clean up the lines pipeline
        device.destroyPipelineLayout(pipelineLayout);
        device.destroyRenderPass(renderPass);

        for (auto imageView: swapChainImageViews) {
            device.destroyImageView(imageView);
        }

        device.destroySwapchainKHR(swapChain);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            device.destroyBuffer(uniformBuffers[i]);
            device.freeMemory(uniformBuffersMemory[i]);
        }

        device.destroyDescriptorPool(descriptorPool); // This will automatically free all descriptor sets
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        device.waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createUniformBuffers(); // Recreate uniform buffers
        createDescriptorPool(); // Recreate descriptor pool
        createDescriptorSets(); // Recreate descriptor sets
        createGraphicsPipeline();
        createDepthResources();
        createFramebuffers();
        createCommandBuffers();
    }

    void initVulkan() {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout(); // Make sure this is before pipeline creation
        createCommandPool();
        createDepthResources();
        createTextureImage("dirt.png");
        createTextureImageView();
        createTextureSampler();
        createUniformBuffers(); // Create uniform buffers before descriptor sets
        createDescriptorPool();
        createDescriptorSets();
        createGraphicsPipeline(); // Create pipeline after descriptor set layout
        createFramebuffers();
        initializeBlocks(); // Initialize blocks before creating their buffers
        createVertexBuffer();
        createIndexBuffer();
        createSelectionBuffer();
        createCommandBuffers();
        createSyncObjects();
    }

    static std::vector<char> readFile(const std::string &filename) {
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

    std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    void createInstance() {
        vk::ApplicationInfo appInfo{};
        appInfo.pApplicationName = "Zerith";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        vk::InstanceCreateInfo createInfo{};
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = 0;
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();

        instance = vk::createInstance(createInfo);
    }

    void createSurface() {
        VkSurfaceKHR _surface;
        if (glfwCreateWindowSurface(instance, window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        surface = vk::SurfaceKHR(_surface);
    }

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
        QueueFamilyIndices indices;
        auto queueFamilies = device.getQueueFamilyProperties();

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphicsFamily = i;
            }

            if (device.getSurfaceSupportKHR(i, surface)) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    void pickPhysicalDevice() {
        auto devices = instance.enumeratePhysicalDevices();
        for (const auto &device: devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (!physicalDevice) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(vk::PhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete();
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies) {
            vk::DeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.wideLines = VK_TRUE;

        deviceFeatures.samplerAnisotropy = VK_TRUE;

        std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vk::DeviceCreateInfo createInfo{};
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        device = physicalDevice.createDevice(createInfo);
        graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
        presentQueue = device.getQueue(indices.presentFamily.value(), 0);
    }

    void createSwapChain() {
        auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
        auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

        vk::SurfaceFormatKHR surfaceFormat = formats[0];
        vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
        vk::Extent2D extent = capabilities.currentExtent;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        swapChain = device.createSwapchainKHR(createInfo);
        swapChainImages = device.getSwapchainImagesKHR(swapChain);
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo createInfo{};
            createInfo.image = swapChainImages[i];
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = vk::ComponentSwizzle::eIdentity;
            createInfo.components.g = vk::ComponentSwizzle::eIdentity;
            createInfo.components.b = vk::ComponentSwizzle::eIdentity;
            createInfo.components.a = vk::ComponentSwizzle::eIdentity;
            createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            swapChainImageViews[i] = device.createImageView(createInfo);
        }
    }

    void createRenderPass() {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentDescription depthAttachment{};
        depthAttachment.format = findDepthFormat();
        depthAttachment.samples = vk::SampleCountFlagBits::e1;
        depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
        depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
        depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

        vk::AttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        vk::SubpassDescription subpass{};
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        vk::SubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.srcAccessMask = vk::AccessFlagBits::eNone;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput |
                                  vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite |
                                   vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        vk::RenderPassCreateInfo renderPassInfo{};
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        renderPass = device.createRenderPass(renderPassInfo);
    }

    vk::ShaderModule createShaderModule(const std::vector<char> &code) {
        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        return device.createShaderModule(createInfo);
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/triangle.vert.spv");
        auto fragShaderCode = readFile("shaders/triangle.frag.spv");

        vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = swapChainExtent;

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack;
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        vk::PipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        pipelineInfo.pDepthStencilState = &depthStencil;

        graphicsPipeline = device.createGraphicsPipeline(nullptr, pipelineInfo).value;

        device.destroyShaderModule(fragShaderModule);
        device.destroyShaderModule(vertShaderModule);

        // Now create the lines pipeline
        auto lineVertShaderCode = readFile("shaders/lines.vert.spv");
        auto lineFragShaderCode = readFile("shaders/lines.frag.spv");

        vk::ShaderModule lineVertShaderModule = createShaderModule(lineVertShaderCode);
        vk::ShaderModule lineFragShaderModule = createShaderModule(lineFragShaderCode);

        vertShaderStageInfo.module = lineVertShaderModule;
        fragShaderStageInfo.module = lineFragShaderModule;

        shaderStages[0] = vertShaderStageInfo;
        shaderStages[1] = fragShaderStageInfo;

        // Modify settings for lines
        auto lineBindingDescription = LineVertex::getBindingDescription();
        auto lineAttributeDescriptions = LineVertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &lineBindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(lineAttributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = lineAttributeDescriptions.data();

        inputAssembly.topology = vk::PrimitiveTopology::eLineList;
        rasterizer.lineWidth = 2.0f;

        // Create lines pipeline using the same pipeline layout
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pRasterizationState = &rasterizer;

        linesPipeline = device.createGraphicsPipeline(nullptr, pipelineInfo).value;

        device.destroyShaderModule(lineFragShaderModule);
        device.destroyShaderModule(lineVertShaderModule);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<vk::ImageView, 2> attachments = {
                swapChainImageViews[i],
                depthImageView
            };

            vk::FramebufferCreateInfo framebufferInfo{};
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        vk::CommandPoolCreateInfo poolInfo{};
        poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        commandPool = device.createCommandPool(poolInfo);
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createVertexBuffer() {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        vertexBuffer = device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(vertexBuffer);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                                   vk::MemoryPropertyFlagBits::eHostCoherent);

        vertexBufferMemory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

        void *data = device.mapMemory(vertexBufferMemory, 0, bufferInfo.size);
        memcpy(data, vertices.data(), (size_t) bufferInfo.size);
        device.unmapMemory(vertexBufferMemory);
    }

    void createIndexBuffer() {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.size = sizeof(indices[0]) * indices.size();
        bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        indexBuffer = device.createBuffer(bufferInfo);

        vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(indexBuffer);

        vk::MemoryAllocateInfo allocInfo{};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                   vk::MemoryPropertyFlagBits::eHostVisible |
                                                   vk::MemoryPropertyFlagBits::eHostCoherent);

        indexBufferMemory = device.allocateMemory(allocInfo);
        device.bindBufferMemory(indexBuffer, indexBufferMemory, 0);

        void *data = device.mapMemory(indexBufferMemory, 0, bufferInfo.size);
        memcpy(data, indices.data(), (size_t) bufferInfo.size);
        device.unmapMemory(indexBufferMemory);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(swapChainImages.size());
        uniformBuffersMemory.resize(swapChainImages.size());
        uniformBuffersMapped.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = bufferSize;
            bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
            bufferInfo.sharingMode = vk::SharingMode::eExclusive;

            uniformBuffers[i] = device.createBuffer(bufferInfo);

            vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(uniformBuffers[i]);

            vk::MemoryAllocateInfo allocInfo{};
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                       vk::MemoryPropertyFlagBits::eHostVisible |
                                                       vk::MemoryPropertyFlagBits::eHostCoherent);

            uniformBuffersMemory[i] = device.allocateMemory(allocInfo);
            device.bindBufferMemory(uniformBuffers[i], uniformBuffersMemory[i], 0);
            uniformBuffersMapped[i] = device.mapMemory(uniformBuffersMemory[i], 0, bufferSize);
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>
                (currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 1000.0f);
        ubo.proj[1][1] *= -1; // Flip Y coordinate for Vulkan

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void createDescriptorSetLayout() {
        std::array<vk::DescriptorSetLayoutBinding, 2> bindings;

        bindings[0].binding = 0;
        bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = vk::ShaderStageFlagBits::eVertex;

        bindings[1].binding = 1;
        bindings[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
    }

    void createDescriptorPool() {
        std::array<vk::DescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
        poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet; // Add this flag

        try {
            descriptorPool = device.createDescriptorPool(poolInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        try {
            descriptorSets = device.allocateDescriptorSets(allocInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::DescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            vk::DescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            device.updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool = commandPool;
        allocInfo.level = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        try {
            commandBuffers = device.allocateCommandBuffers(allocInfo);
        } catch (vk::SystemError &err) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            vk::CommandBufferBeginInfo beginInfo{};

            try {
                commandBuffers[i].begin(beginInfo);
            } catch (vk::SystemError &err) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            vk::RenderPassBeginInfo renderPassInfo{};
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            std::array<vk::ClearValue, 2> clearValues{};
            clearValues[0].color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
            commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

            if (i < descriptorSets.size()) {
                // Add safety check
                commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                     pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
            }

            if (vertexBuffer && indexBuffer) {
                // Add safety checks
                vk::Buffer vertexBuffers[] = {vertexBuffer};
                vk::DeviceSize offsets[] = {0};
                commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
                commandBuffers[i].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

                if (!indices.empty()) {
                    // Add safety check
                    commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
                }
            }

            if (selectedBlock.hasSelection && selectionBuffer) {
                // Add safety check
                commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, linesPipeline);
                vk::Buffer selectionBuffers[] = {selectionBuffer};
                vk::DeviceSize selectionOffsets[] = {0};
                commandBuffers[i].bindVertexBuffers(0, 1, selectionBuffers, selectionOffsets);
                commandBuffers[i].draw(24, 1, 0, 0);
            }

            commandBuffers[i].endRenderPass();

            try {
                commandBuffers[i].end();
            } catch (vk::SystemError &err) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        vk::SemaphoreCreateInfo semaphoreInfo{};
        vk::FenceCreateInfo fenceInfo{};
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
            renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
            inFlightFences[i] = device.createFence(fenceInfo);
        }
    }

    void drawFrame() {
        device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        try {
            auto result = device.acquireNextImageKHR(swapChain, UINT64_MAX,
                                                     imageAvailableSemaphores[currentFrame], nullptr);
            imageIndex = result.value;
        } catch (vk::OutOfDateKHRError &e) {
            return;
        }

        device.resetFences(1, &inFlightFences[currentFrame]);

        vk::SubmitInfo submitInfo{};
        vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        bool needsRecreation = false;
        try {
            auto result = presentQueue.presentKHR(presentInfo);
        } catch (vk::OutOfDateKHRError &e) {
            needsRecreation = true;
        }

        if (needsRecreation || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            processInput();

            // Ensure the cursor remains disabled
            if (cursorLocked) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }

            handleBlockInteraction();
            drawFrame();
        }
        device.waitIdle();
    }

    void cleanup() {
        cleanupSwapChain();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device.destroySemaphore(imageAvailableSemaphores[i]);
            device.destroySemaphore(renderFinishedSemaphores[i]);
            device.destroyFence(inFlightFences[i]);
        }

        device.destroySampler(textureSampler);
        device.destroyImageView(textureImageView);
        device.destroyImage(textureImage);
        device.freeMemory(textureImageMemory);

        device.destroyImageView(depthImageView);
        device.destroyImage(depthImage);
        device.freeMemory(depthImageMemory);

        device.destroyCommandPool(commandPool);

        device.destroyBuffer(vertexBuffer);
        device.freeMemory(vertexBufferMemory);
        device.destroyBuffer(indexBuffer);
        device.freeMemory(indexBufferMemory);

        device.destroyBuffer(selectionBuffer);
        device.freeMemory(selectionBufferMemory);
        device.destroyPipeline(linesPipeline);

        for (size_t i = 0; i < uniformBuffers.size(); i++) {
            device.destroyBuffer(uniformBuffers[i]);
            device.freeMemory(uniformBuffersMemory[i]);
        }

        device.destroyDescriptorPool(descriptorPool);
        device.destroyDescriptorSetLayout(descriptorSetLayout);

        device.destroy();
        instance.destroySurfaceKHR(surface);
        instance.destroy();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    Zerith app;

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
