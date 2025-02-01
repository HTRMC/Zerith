// ShaderManager.hpp
#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class ShaderManager {
public:
    static ShaderManager& getInstance() {
        static ShaderManager instance;
        return instance;
    }

    void init(VkDevice device);
    void cleanup();

    VkShaderModule getShader(const std::string& filename);
    std::pair<VkShaderModule, VkShaderModule> getShaderPair(const std::string& vertFilename,
                                                           const std::string& fragFilename);

private:
    ShaderManager() = default;
    ~ShaderManager() = default;
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    VkShaderModule createShaderModule(const std::vector<char>& code);
    static std::vector<char> readFile(const std::string& filename);
    void addShader(const std::string& filename);

    VkDevice device = VK_NULL_HANDLE;
    std::unordered_map<std::string, VkShaderModule> shaderModules;
    std::string shaderPath = "shaders/"; // Default shader directory
};

