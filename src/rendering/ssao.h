#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>

class SSAO {
public:
    SSAO() = default;
    ~SSAO() = default;

    struct SSAOData {
        glm::mat4 projection;
        glm::mat4 view;
        glm::vec4 samples[64];
        glm::vec4 noiseScale;
    };

    struct PushConstants {
        float radius = 0.5f;
        float bias = 0.025f;
        float intensity = 1.0f;
        uint32_t kernelSize = 64;
    };

    void init(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent);
    void cleanup(VkDevice device);
    
    void createSSAOResources(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent);
    void createNoiseTexture(VkDevice device, VkPhysicalDevice physicalDevice);
    void generateKernel();
    
    VkImageView getSSAOImageView() const { return ssaoImageView; }
    VkImageView getSSAOBlurImageView() const { return ssaoBlurImageView; }
    
    const std::vector<glm::vec4>& getKernel() const { return ssaoKernel; }
    glm::vec4 getNoiseScale(VkExtent2D extent) const {
        return glm::vec4(extent.width / 4.0f, extent.height / 4.0f, 0.0f, 0.0f);
    }

private:
    // SSAO render targets
    VkImage ssaoImage = VK_NULL_HANDLE;
    VkDeviceMemory ssaoImageMemory = VK_NULL_HANDLE;
    VkImageView ssaoImageView = VK_NULL_HANDLE;
    
    VkImage ssaoBlurImage = VK_NULL_HANDLE;
    VkDeviceMemory ssaoBlurImageMemory = VK_NULL_HANDLE;
    VkImageView ssaoBlurImageView = VK_NULL_HANDLE;
    
    // Noise texture
    VkImage noiseImage = VK_NULL_HANDLE;
    VkDeviceMemory noiseImageMemory = VK_NULL_HANDLE;
    VkImageView noiseImageView = VK_NULL_HANDLE;
    VkSampler noiseSampler = VK_NULL_HANDLE;
    
    // SSAO kernel
    std::vector<glm::vec4> ssaoKernel;
    std::vector<glm::vec3> ssaoNoise;
    
    // Helper functions
    void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
                    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    float lerp(float a, float b, float f) {
        return a + f * (b - a);
    }
};