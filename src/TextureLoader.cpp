#include "TextureLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <stdexcept>

TextureLoader::TextureLoader(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue) {
    init(device, physicalDevice, commandPool, graphicsQueue);
}

TextureLoader::~TextureLoader() {
    cleanup();
}

void TextureLoader::init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue) {
    this->device = device;
    this->physicalDevice = physicalDevice;
    this->commandPool = commandPool;
    this->graphicsQueue = graphicsQueue;

    createTextureSampler();
    createDefaultTexture();
}

uint32_t TextureLoader::loadTexture(const std::string& filename) {
    // Check if texture is already loaded
    auto it = texturePathToId.find(filename);
    if (it != texturePathToId.end()) {
        return it->second;
    }

    // Create new texture
    Texture texture;
    try {
        createTextureImage(filename, texture);
        createTextureImageView(texture);
        
        // Add to collection
        uint32_t textureId = static_cast<uint32_t>(textures.size());
        textures.push_back(texture);
        texturePathToId[filename] = textureId;
        
        std::cout << "Loaded texture: " << filename << " (ID: " << textureId << ")" << std::endl;
        return textureId;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load texture " << filename << ": " << e.what() << std::endl;
        std::cerr << "Using default texture instead" << std::endl;
        return getDefaultTextureId();
    }
}

VkImageView TextureLoader::getTextureImageView(uint32_t textureId) const {
    if (textureId < textures.size()) {
        return textures[textureId].imageView;
    }
    return textures[defaultTextureId].imageView;
}

VkSampler TextureLoader::getTextureSampler() const {
    return textureSampler;
}

bool TextureLoader::hasTexture(uint32_t textureId) const {
    return textureId < textures.size();
}

uint32_t TextureLoader::getDefaultTextureId() const {
    return defaultTextureId;
}

void TextureLoader::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Get physical device features to check anisotropy support
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    if (deviceFeatures.samplerAnisotropy) {
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    } else {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

void TextureLoader::createDefaultTexture() {
    // Create a simple checkerboard pattern for the default texture
    const int width = 16;
    const int height = 16;
    const int channels = 4;  // RGBA
    
    // Create checkerboard pattern
    unsigned char pixels[width * height * channels];
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            bool isWhite = (x < width / 2 && y < height / 2) || (x >= width / 2 && y >= height / 2);
            unsigned char color = isWhite ? 255 : 128;
            
            int index = (y * width + x) * channels;
            pixels[index + 0] = color;    // R
            pixels[index + 1] = 0;        // G
            pixels[index + 2] = color;    // B
            pixels[index + 3] = 255;      // A
        }
    }
    
    VkDeviceSize imageSize = width * height * channels;
    
    // Create a staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer for default texture!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory for default texture!");
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
    
    // Copy pixel data to buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    
    // Create the default texture struct
    Texture defaultTexture;
    defaultTexture.width = width;
    defaultTexture.height = height;
    defaultTexture.channels = channels;
    defaultTexture.path = "default_texture";
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &defaultTexture.image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create default texture image!");
    }
    
    vkGetImageMemoryRequirements(device, defaultTexture.image, &memRequirements);
    
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &defaultTexture.memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate default texture image memory!");
    }
    
    vkBindImageMemory(device, defaultTexture.image, defaultTexture.memory, 0);
    
    // Transition image and copy data
    transitionImageLayout(defaultTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, defaultTexture.image, width, height);
    transitionImageLayout(defaultTexture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Create image view
    createTextureImageView(defaultTexture);
    
    // Add to textures list
    textures.push_back(defaultTexture);
    defaultTextureId = 0;  // First texture is the default
    
    // Clean up staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    std::cout << "Created default texture" << std::endl;
}

void TextureLoader::createTextureImage(const std::string& filename, Texture& texture) {
    // Load image with stb_image
    int texWidth, texHeight, texChannels;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4; // 4 bytes per pixel for RGBA
    
    if (!pixels) {
        throw std::runtime_error("Failed to load texture image: " + filename);
    }
    
    // Save dimensions and path
    texture.width = texWidth;
    texture.height = texHeight;
    texture.channels = 4; // We requested RGBA
    texture.path = filename;
    
    std::cout << "Loaded image: " << filename << " (" << texWidth << "x" << texHeight << ")" << std::endl;
    
    // Create a staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    // Create buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to create staging buffer!");
    }
    
    // Get memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
    
    // Allocate memory
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
    
    // Copy pixel data to buffer
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    
    // Clean up pixel data
    stbi_image_free(pixels);
    
    // Create the texture image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &texture.image) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        throw std::runtime_error("Failed to create texture image!");
    }
    
    // Get memory requirements for the image
    vkGetImageMemoryRequirements(device, texture.image, &memRequirements);
    
    // Allocate memory for the image
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &texture.memory) != VK_SUCCESS) {
        vkDestroyImage(device, texture.image, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        throw std::runtime_error("Failed to allocate texture image memory!");
    }
    
    vkBindImageMemory(device, texture.image, texture.memory, 0);
    
    // Transition image to be optimal for receiving data
    transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // Copy data from buffer to image
    copyBufferToImage(stagingBuffer, texture.image, 
                      static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    
    // Transition image to be optimal for shader access
    transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // Clean up staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void TextureLoader::createTextureImageView(Texture& texture) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &texture.imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view!");
    }
}

uint32_t TextureLoader::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

void TextureLoader::transitionImageLayout(VkImage image, VkFormat format, 
                                         VkImageLayout oldLayout, VkImageLayout newLayout) {
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
        throw std::runtime_error("Unsupported layout transition!");
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

void TextureLoader::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
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

VkCommandBuffer TextureLoader::beginSingleTimeCommands() {
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

void TextureLoader::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void TextureLoader::cleanup() {
    if (device != VK_NULL_HANDLE) {
        if (textureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, textureSampler, nullptr);
            textureSampler = VK_NULL_HANDLE;
        }
        
        for (auto& texture : textures) {
            if (texture.imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, texture.imageView, nullptr);
                texture.imageView = VK_NULL_HANDLE;
            }
            if (texture.image != VK_NULL_HANDLE) {
                vkDestroyImage(device, texture.image, nullptr);
                texture.image = VK_NULL_HANDLE;
            }
            if (texture.memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, texture.memory, nullptr);
                texture.memory = VK_NULL_HANDLE;
            }
        }
        
        textures.clear();
        texturePathToId.clear();
    }
}

VkDescriptorImageInfo TextureLoader::createTextureArray(const std::vector<std::string>& filenames) {
    if (filenames.empty()) {
        throw std::runtime_error("Cannot create texture array with no textures");
    }

    // Clean up existing texture array if any
    if (hasTextureArray) {
        if (textureArray.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, textureArray.imageView, nullptr);
            textureArray.imageView = VK_NULL_HANDLE;
        }
        if (textureArray.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, textureArray.image, nullptr);
            textureArray.image = VK_NULL_HANDLE;
        }
        if (textureArray.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, textureArray.memory, nullptr);
            textureArray.memory = VK_NULL_HANDLE;
        }
    }

    // Log which textures we're loading
    std::cout << "Creating texture array with " << filenames.size() << " textures:" << std::endl;
    for (const auto& name : filenames) {
        std::cout << "  - " << name << std::endl;
    }

    // Load all textures to get dimensions and pixel data
    struct TextureData {
        int width;
        int height;
        int channels;
        unsigned char* pixels;
    };

    std::vector<TextureData> loadedTextures;
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;

    stbi_set_flip_vertically_on_load(true);

    // First pass: load all textures and find maximum dimensions
    for (const auto& filename : filenames) {
        TextureData data;
        data.pixels = stbi_load(filename.c_str(), &data.width, &data.height, &data.channels, STBI_rgb_alpha);

        if (!data.pixels) {
            // Clean up any previously loaded textures
            for (auto& tex : loadedTextures) {
                stbi_image_free(tex.pixels);
            }
            throw std::runtime_error("Failed to load texture image: " + filename);
        }

        loadedTextures.push_back(data);

        // Track the maximum dimensions
        maxWidth = std::max(maxWidth, static_cast<uint32_t>(data.width));
        maxHeight = std::max(maxHeight, static_cast<uint32_t>(data.height));

        std::cout << "Loaded texture for array: " << filename << " (" << data.width << "x" << data.height << ")" << std::endl;
    }

    // Create the texture array image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = maxWidth;
    imageInfo.extent.height = maxHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = static_cast<uint32_t>(filenames.size());
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    if (vkCreateImage(device, &imageInfo, nullptr, &textureArray.image) != VK_SUCCESS) {
        // Clean up loaded textures
        for (auto& tex : loadedTextures) {
            stbi_image_free(tex.pixels);
        }
        throw std::runtime_error("Failed to create texture array image!");
    }

    // Get memory requirements and allocate memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, textureArray.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(device, &allocInfo, nullptr, &textureArray.memory) != VK_SUCCESS) {
        // Clean up loaded textures
        for (auto& tex : loadedTextures) {
            stbi_image_free(tex.pixels);
        }
        vkDestroyImage(device, textureArray.image, nullptr);
        textureArray.image = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to allocate texture array memory!");
    }

    vkBindImageMemory(device, textureArray.image, textureArray.memory, 0);

    // Transition the image layout for copy operations
    transitionImageLayout(textureArray.image, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy data from each loaded texture to the texture array
    for (size_t i = 0; i < loadedTextures.size(); i++) {
        const auto& tex = loadedTextures[i];

        // Create a staging buffer for this texture
        VkDeviceSize imageSize = maxWidth * maxHeight * 4; // 4 bytes per pixel (RGBA)

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            for (auto& t : loadedTextures) {
                stbi_image_free(t.pixels);
            }
            throw std::runtime_error("Failed to create staging buffer for texture array!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            for (auto& t : loadedTextures) {
                stbi_image_free(t.pixels);
            }
            throw std::runtime_error("Failed to allocate staging buffer memory for texture array!");
        }

        vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

        // Copy texture data to staging buffer
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, tex.pixels, imageSize);
        vkUnmapMemory(device, stagingBufferMemory);

        // Copy from staging buffer to image array layer
        copyBufferToImageArray(stagingBuffer, textureArray.image, maxWidth, maxHeight, static_cast<uint32_t>(i));

        // Clean up staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        std::cout << "Successfully loaded texture: " << filenames[i] << " into array layer " << i << std::endl;
    }

    // Transition layout for shader access
    transitionImageLayout(textureArray.image, VK_FORMAT_R8G8B8A8_SRGB,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Create image view for the texture array
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureArray.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = static_cast<uint32_t>(filenames.size());

    if (vkCreateImageView(device, &viewInfo, nullptr, &textureArray.imageView) != VK_SUCCESS) {
        for (auto& tex : loadedTextures) {
            stbi_image_free(tex.pixels);
        }
        throw std::runtime_error("Failed to create texture array image view!");
    }

    // Clean up loaded textures
    for (auto& tex : loadedTextures) {
        stbi_image_free(tex.pixels);
    }

    // Update texture array metadata
    textureArray.layerCount = static_cast<uint32_t>(filenames.size());
    textureArray.width = maxWidth;
    textureArray.height = maxHeight;
    hasTextureArray = true;

    std::cout << "Created texture array with " << filenames.size() << " layers" << std::endl;

    // Return the descriptor image info for this texture array
    VkDescriptorImageInfo descriptorInfo{};
    descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorInfo.imageView = textureArray.imageView;
    descriptorInfo.sampler = textureSampler;

    return descriptorInfo;
}

void TextureLoader::copyBufferToImageArray(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerIndex) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layerIndex;
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