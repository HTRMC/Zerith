// TextureLoader.hpp
#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Forward declarations
class VulkanApp;

class TextureLoader {
public:
    TextureLoader() = default;
    TextureLoader(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~TextureLoader();

    // Initialize the texture loader
    void init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);

    // Load a texture and return its ID
    uint32_t loadTexture(const std::string& filename);
    
    // Create a texture array from multiple textures
    VkDescriptorImageInfo createTextureArray(const std::vector<std::string>& filenames);

    // Get texture image view
    VkImageView getTextureImageView(uint32_t textureId) const;
    
    // Get texture sampler
    VkSampler getTextureSampler() const;
    
    // Check if a texture ID is valid
    bool hasTexture(uint32_t textureId) const;
    
    // Get the default texture ID
    uint32_t getDefaultTextureId() const;

    // Get the texture array image view
    VkImageView getTextureArrayImageView() const;

    void cleanup();

private:
    struct Texture {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
        std::string path;
    };

    struct TextureArray {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        uint32_t layerCount = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    
    std::unordered_map<std::string, uint32_t> texturePathToId;
    std::vector<Texture> textures;
    uint32_t defaultTextureId = 0;

    // Texture array data
    TextureArray textureArray;
    bool hasTextureArray = false;

    void createTextureSampler();
    void createTextureImage(const std::string& filename, Texture& texture);
    void createTextureImageView(Texture& texture);
    void createDefaultTexture();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void copyBufferToImageArray(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerIndex);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
};