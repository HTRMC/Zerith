#include "texture_array.h"
#include <iostream>

namespace MeshShader {

TextureArray::TextureArray() {
    initializeTextureMap();
}

void TextureArray::initializeTextureMap() {
    // Define the texture layers
    // Order matters! This determines the layer index
    m_textureFiles = {
        "assets/oak_planks.png",           // Layer 0
        "assets/stone.png",                // Layer 1
        "assets/dirt.png",                 // Layer 2
        "assets/grass_block_top.png",      // Layer 3
        "assets/grass_block_side.png",     // Layer 4
        // We can add more textures here as needed
    };
    
    // Build the name-to-layer map for different block types
    // Oak planks block
    m_textureIndices["oak_planks_all"] = 0;
    
    // Stone block
    m_textureIndices["stone_all"] = 1;
    
    // Grass block faces
    m_textureIndices["grass_bottom"] = 2;  // Dirt
    m_textureIndices["grass_top"] = 3;     // Grass top
    m_textureIndices["grass_side"] = 4;    // Grass side
    
    // Oak slab (same as planks)
    m_textureIndices["oak_slab_all"] = 0;
    
    // Oak stairs (same as planks)
    m_textureIndices["oak_stairs_all"] = 0;
}

uint32_t TextureArray::getTextureLayer(const std::string& textureName) const {
    auto it = m_textureIndices.find(textureName);
    if (it != m_textureIndices.end()) {
        return it->second;
    }
    
    std::cerr << "Warning: Texture '" << textureName << "' not found, using default" << std::endl;
    return 0; // Default to oak planks
}

} // namespace MeshShader