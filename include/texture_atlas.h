#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace Zerith {

enum class TextureID : uint32_t {
    OAK_PLANKS = 0,
    STONE = 1,
    DIRT = 2,
    GRASS_TOP = 3,
    GRASS_SIDE = 4,
    OAK_SLAB = 5,
    OAK_STAIRS = 6,
    
    ALL = OAK_PLANKS,
    TOP = GRASS_TOP,
    SIDE = GRASS_SIDE,
    BOTTOM = DIRT,
    
    COUNT
};

constexpr uint32_t TEXTURE_COUNT = static_cast<uint32_t>(TextureID::COUNT);

// Structure to hold texture region information in the atlas
struct TextureRegion {
    glm::vec2 uvMin;  // Top-left UV coordinates
    glm::vec2 uvMax;  // Bottom-right UV coordinates
    
    TextureRegion() : uvMin(0.0f), uvMax(1.0f) {}
    TextureRegion(float minU, float minV, float maxU, float maxV) 
        : uvMin(minU, minV), uvMax(maxU, maxV) {}
};

class TextureAtlas {
public:
    static constexpr int TEXTURE_SIZE = 16;      // Each texture is 16x16
    static constexpr int ATLAS_WIDTH = 256;      // Atlas is 256x256 (16x16 textures)
    static constexpr int ATLAS_HEIGHT = 256;
    static constexpr int TEXTURES_PER_ROW = ATLAS_WIDTH / TEXTURE_SIZE;
    
    TextureAtlas();
    
    // Get UV coordinates for a specific texture (optimized integer ID version)
    TextureRegion getTextureRegion(TextureID textureID) const;
    
    // Get UV coordinates for a specific texture (legacy string version)
    TextureRegion getTextureRegion(const std::string& textureName) const;
    
    // Get the texture index for a given ID (optimized version)
    uint32_t getTextureIndex(TextureID textureID) const;
    
    // Get the texture index for a given name (legacy version)
    int getTextureIndex(const std::string& textureName) const;
    
    // Convert texture coordinates from 0-16 range to atlas UV coordinates (optimized version)
    glm::vec4 convertToAtlasUV(const glm::vec4& blockUV, TextureID textureID) const;
    
    // Convert texture coordinates from 0-16 range to atlas UV coordinates (legacy version)
    glm::vec4 convertToAtlasUV(const glm::vec4& blockUV, const std::string& textureName) const;
    
    // Get the list of texture files to load
    const std::vector<std::string>& getTextureFiles() const { return m_textureFiles; }
    
private:
    // Map from texture name to index in the atlas
    std::unordered_map<std::string, int> m_textureIndices;
    
    // List of texture file paths in order
    std::vector<std::string> m_textureFiles;
    
    // Precalculated texture regions
    std::vector<TextureRegion> m_textureRegions;
    
    void initializeTextureMap();
    TextureRegion calculateRegion(int index) const;
};

} // namespace Zerith