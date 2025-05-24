#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace MeshShader {

class TextureArray {
public:
    static constexpr int TEXTURE_SIZE = 16;  // Each texture is 16x16
    
    TextureArray();
    
    // Get the texture layer index for a given texture name
    uint32_t getTextureLayer(const std::string& textureName) const;
    
    // Get the list of texture files to load in order
    const std::vector<std::string>& getTextureFiles() const { return m_textureFiles; }
    
    // Get total number of texture layers
    size_t getLayerCount() const { return m_textureFiles.size(); }
    
private:
    // Map from texture name to layer index
    std::unordered_map<std::string, uint32_t> m_textureIndices;
    
    // List of texture file paths in order (each becomes a layer)
    std::vector<std::string> m_textureFiles;
    
    void initializeTextureMap();
};

} // namespace MeshShader