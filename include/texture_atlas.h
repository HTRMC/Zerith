#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace MeshShader {

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
    
    // Get UV coordinates for a specific texture
    TextureRegion getTextureRegion(const std::string& textureName) const;
    
    // Get the texture index for a given name
    int getTextureIndex(const std::string& textureName) const;
    
    // Convert texture coordinates from 0-16 range to atlas UV coordinates
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

} // namespace MeshShader