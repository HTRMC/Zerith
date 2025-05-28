#include "texture_atlas.h"
#include <iostream>

namespace Zerith {

TextureAtlas::TextureAtlas() {
    initializeTextureMap();
}

void TextureAtlas::initializeTextureMap() {
    // Define the texture layout in the atlas
    // Order matters! This determines the position in the atlas
    m_textureFiles = {
        "assets/oak_planks.png",           // Index 0
        "assets/stone.png",                // Index 1
        "assets/dirt.png",                 // Index 2
        "assets/grass_block_top.png",      // Index 3
        "assets/grass_block_side.png",     // Index 4
        "assets/oak_planks.png",           // Index 5 (slab uses same texture)
        "assets/oak_planks.png"            // Index 6 (stairs uses same texture)
    };
    
    // Build the name-to-index map
    m_textureIndices["oak_planks"] = 0;
    m_textureIndices["stone"] = 1;
    m_textureIndices["dirt"] = 2;
    m_textureIndices["grass_top"] = 3;
    m_textureIndices["grass_side"] = 4;
    m_textureIndices["oak_slab"] = 5;
    m_textureIndices["oak_stairs"] = 6;
    
    // Special mappings for block faces
    m_textureIndices["#all"] = 0;  // Default texture
    m_textureIndices["#top"] = 3;  // Grass top
    m_textureIndices["#side"] = 4; // Grass side
    m_textureIndices["#bottom"] = 2; // Dirt
    
    // Precalculate texture regions
    for (int i = 0; i < m_textureFiles.size(); ++i) {
        m_textureRegions.emplace_back(calculateRegion(i));
    }
}

TextureRegion TextureAtlas::calculateRegion(int index) const {
    int row = index / TEXTURES_PER_ROW;
    int col = index % TEXTURES_PER_ROW;
    
    float texWidth = 1.0f / TEXTURES_PER_ROW;
    float texHeight = 1.0f / TEXTURES_PER_ROW;
    
    float minU = col * texWidth;
    float minV = row * texHeight;
    float maxU = minU + texWidth;
    float maxV = minV + texHeight;
    
    return TextureRegion(minU, minV, maxU, maxV);
}

TextureRegion TextureAtlas::getTextureRegion(const std::string& textureName) const {
    auto it = m_textureIndices.find(textureName);
    if (it != m_textureIndices.end() && it->second < m_textureRegions.size()) {
        return m_textureRegions[it->second];
    }
    
    // Return default texture region (oak planks)
    return m_textureRegions[0];
}

int TextureAtlas::getTextureIndex(const std::string& textureName) const {
    auto it = m_textureIndices.find(textureName);
    if (it != m_textureIndices.end()) {
        return it->second;
    }
    return 0; // Default to oak planks
}

glm::vec4 TextureAtlas::convertToAtlasUV(const glm::vec4& blockUV, const std::string& textureName) const {
    TextureRegion region = getTextureRegion(textureName);
    
    // Convert from 0-16 block coordinates to 0-1 normalized coordinates
    float u1 = blockUV.x / 16.0f;
    float v1 = blockUV.y / 16.0f;
    float u2 = blockUV.z / 16.0f;
    float v2 = blockUV.w / 16.0f;
    
    // Map to the texture region in the atlas
    float atlasU1 = region.uvMin.x + u1 * (region.uvMax.x - region.uvMin.x);
    float atlasV1 = region.uvMin.y + v1 * (region.uvMax.y - region.uvMin.y);
    float atlasU2 = region.uvMin.x + u2 * (region.uvMax.x - region.uvMin.x);
    float atlasV2 = region.uvMin.y + v2 * (region.uvMax.y - region.uvMin.y);
    
    return glm::vec4(atlasU1, atlasV1, atlasU2, atlasV2);
}

} // namespace Zerith