#include "texture_array.h"
#include "logger.h"
#include <algorithm>

namespace Zerith {

TextureArray::TextureArray() {
    LOG_DEBUG("Initializing TextureArray with runtime registration");
    initializeTextureMap();
}

void TextureArray::initializeTextureMap() {
    // Register the default textures that the game needs
    // These will be at predictable indices for compatibility
    registerTexture("assets/oak_planks.png");           // Layer 0
    registerTexture("assets/stone.png");                // Layer 1
    registerTexture("assets/dirt.png");                 // Layer 2
    registerTexture("assets/grass_block_top.png");      // Layer 3
    registerTexture("assets/grass_block_side.png");     // Layer 4
    registerTexture("assets/grass_block_side_overlay.png"); // Layer 5
    
    // Build the name-to-layer map for different block types
    // These mappings are for compatibility with existing code
    // New textures will automatically get their name from the filename
    
    // Oak planks block
    m_textureIndices["oak_planks_all"] = 0;
    m_textureIndices["oak_planks"] = 0;
    
    // Stone block
    m_textureIndices["stone"] = 1;
    
    // Dirt block
    m_textureIndices["dirt"] = 2;
    
    // Grass block faces
    m_textureIndices["grass_bottom"] = 2;  // Dirt
    m_textureIndices["grass_top"] = 3;     // Grass top
    m_textureIndices["grass_block_top"] = 3;
    m_textureIndices["grass_side"] = 4;    // Grass side
    m_textureIndices["grass_block_side"] = 4;
    m_textureIndices["grass_overlay"] = 5;  // Grass side overlay
    m_textureIndices["grass_block_side_overlay"] = 5;
    
    // Oak slab (same as planks)
    m_textureIndices["oak_slab_all"] = 0;
    
    // Oak stairs (same as planks)
    m_textureIndices["oak_stairs_all"] = 0;
}

TextureLayer TextureArray::registerTexture(const std::string& texturePath) {
    // Check if already registered
    auto it = m_pathToLayer.find(texturePath);
    if (it != m_pathToLayer.end()) {
        return it->second;
    }
    
    // Register new texture
    TextureLayer layer = m_nextLayer++;
    m_textureFiles.push_back(texturePath);
    m_pathToLayer[texturePath] = layer;
    
    // Extract texture name from path (e.g., "assets/deepslate.png" -> "deepslate")
    std::string textureName = texturePath;
    size_t lastSlash = textureName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        textureName = textureName.substr(lastSlash + 1);
    }
    size_t dot = textureName.find_last_of(".");
    if (dot != std::string::npos) {
        textureName = textureName.substr(0, dot);
    }
    
    // Register the texture name mapping
    m_textureIndices[textureName] = layer;
    
    LOG_DEBUG("Registered texture '%s' at layer %u", texturePath.c_str(), layer);
    return layer;
}

TextureLayer TextureArray::getOrRegisterTexture(const std::string& texturePath) {
    return registerTexture(texturePath);
}

uint32_t TextureArray::getTextureLayer(const std::string& textureName) const {
    auto it = m_textureIndices.find(textureName);
    if (it != m_textureIndices.end()) {
        return it->second;
    }
    
    LOG_WARN("Texture '%s' not found, using default", textureName.c_str());
    return 0; // Default to oak planks
}

bool TextureArray::hasTexture(const std::string& texturePath) const {
    return m_pathToLayer.find(texturePath) != m_pathToLayer.end();
}

} // namespace Zerith