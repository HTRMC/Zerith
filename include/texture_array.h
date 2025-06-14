#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace Zerith {

// TextureLayer enum removed - now using runtime registration
using TextureLayer = uint32_t;

class TextureArray {
public:
    static constexpr int TEXTURE_SIZE = 16;  // Each texture is 16x16
    static constexpr uint32_t MISSING_TEXTURE_LAYER = 0xFFFFFFFF;  // Special layer ID for missing textures
    
    TextureArray();
    
    // Register a new texture at runtime and return its layer index
    TextureLayer registerTexture(const std::string& texturePath);
    
    // Get or register a texture - if it doesn't exist, registers it
    TextureLayer getOrRegisterTexture(const std::string& texturePath);
    
    // Get the texture layer index for a given texture name
    uint32_t getTextureLayer(const std::string& textureName) const;
    
    // Get the texture layer index for a given texture path
    uint32_t getTextureLayerByPath(const std::string& texturePath) const;
    
    // Get the list of texture files to load in order
    const std::vector<std::string>& getTextureFiles() const { return m_textureFiles; }
    
    // Get total number of texture layers
    size_t getLayerCount() const { return m_textureFiles.size(); }
    
    // Check if a texture is already registered
    bool hasTexture(const std::string& texturePath) const;
    
private:
    // Map from texture name to layer index
    std::unordered_map<std::string, uint32_t> m_textureIndices;
    
    // Map from texture path to layer index (for fast lookup)
    std::unordered_map<std::string, uint32_t> m_pathToLayer;
    
    // List of texture file paths in order (each becomes a layer)
    std::vector<std::string> m_textureFiles;
    
    // Next available layer index
    uint32_t m_nextLayer = 0;
    
    void initializeTextureMap();
};

} // namespace Zerith