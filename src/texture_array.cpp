#include "texture_array.h"
#include "logger.h"
#include <algorithm>

namespace Zerith {

TextureArray::TextureArray() {
    LOG_DEBUG("Initializing TextureArray with runtime registration");
    initializeTextureMap();
}

void TextureArray::initializeTextureMap() {
    // Empty - textures are now loaded dynamically from models
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
    
    LOG_WARN("Texture '%s' not found, using missing texture pattern", textureName.c_str());
    return MISSING_TEXTURE_LAYER;
}

uint32_t TextureArray::getTextureLayerByPath(const std::string& texturePath) const {
    auto it = m_pathToLayer.find(texturePath);
    if (it != m_pathToLayer.end()) {
        return it->second;
    }
    
    LOG_WARN("Texture path '%s' not found, using missing texture pattern", texturePath.c_str());
    return MISSING_TEXTURE_LAYER;
}

bool TextureArray::hasTexture(const std::string& texturePath) const {
    return m_pathToLayer.find(texturePath) != m_pathToLayer.end();
}

} // namespace Zerith