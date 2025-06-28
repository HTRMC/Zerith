#include "ssao.h"
#include <stdexcept>
#include <cstring>

void SSAO::init(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent) {
    generateKernel();
    createSSAOResources(device, physicalDevice, extent);
    createNoiseTexture(device, physicalDevice);
}

void SSAO::cleanup(VkDevice device) {
    if (noiseSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, noiseSampler, nullptr);
    }
    
    if (noiseImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, noiseImageView, nullptr);
    }
    
    if (noiseImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, noiseImage, nullptr);
        vkFreeMemory(device, noiseImageMemory, nullptr);
    }
    
    if (ssaoBlurImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, ssaoBlurImageView, nullptr);
    }
    
    if (ssaoBlurImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, ssaoBlurImage, nullptr);
        vkFreeMemory(device, ssaoBlurImageMemory, nullptr);
    }
    
    if (ssaoImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, ssaoImageView, nullptr);
    }
    
    if (ssaoImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, ssaoImage, nullptr);
        vkFreeMemory(device, ssaoImageMemory, nullptr);
    }
}

void SSAO::generateKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    
    ssaoKernel.resize(64);
    for (int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        
        // Scale samples so they're more aligned to center of kernel
        float scale = float(i) / 64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        
        ssaoKernel[i] = glm::vec4(sample, 0.0f);
    }
    
    // Generate noise texture data
    ssaoNoise.resize(16);
    for (int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        ssaoNoise[i] = noise;
    }
}

void SSAO::createSSAOResources(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent) {
    // Create SSAO image (R8 format for single channel)
    createImage(device, physicalDevice, extent.width, extent.height,
                VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                ssaoImage, ssaoImageMemory);
    
    ssaoImageView = createImageView(device, ssaoImage, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    
    // Create SSAO blur image
    createImage(device, physicalDevice, extent.width, extent.height,
                VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                ssaoBlurImage, ssaoBlurImageMemory);
    
    ssaoBlurImageView = createImageView(device, ssaoBlurImage, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void SSAO::createNoiseTexture(VkDevice device, VkPhysicalDevice physicalDevice) {
    // Create 4x4 noise texture
    createImage(device, physicalDevice, 4, 4,
                VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                noiseImage, noiseImageMemory);
    
    // Transition image layout and copy noise data
    // Note: This would need to be done in a command buffer with proper synchronization
    // For brevity, I'm omitting the staging buffer and copy operations here
    
    noiseImageView = createImageView(device, noiseImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &noiseSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create noise sampler!");
    }
}

void SSAO::createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
                      VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView SSAO::createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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

uint32_t SSAO::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}