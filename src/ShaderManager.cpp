// ShaderManager.cpp
#include "ShaderManager.hpp"
#include <fstream>
#include <stdexcept>

void ShaderManager::init(VkDevice device) {
    this->device = device;
}

void ShaderManager::cleanup() {
    if (device) {
        for (auto& [name, module] : shaderModules) {
            vkDestroyShaderModule(device, module, nullptr);
        }
        shaderModules.clear();
    }
}

VkShaderModule ShaderManager::getShader(const std::string& filename) {
    // Check if shader is already loaded
    auto it = shaderModules.find(filename);
    if (it != shaderModules.end()) {
        return it->second;
    }

    // If not, load and cache it
    addShader(filename);
    return shaderModules[filename];
}

std::pair<VkShaderModule, VkShaderModule> ShaderManager::getShaderPair(
    const std::string& vertFilename, const std::string& fragFilename) {
    return {
        getShader(vertFilename),
        getShader(fragFilename)
    };
}

void ShaderManager::addShader(const std::string& filename) {
    std::string fullPath = shaderPath + filename;
    auto code = readFile(fullPath);
    VkShaderModule shaderModule = createShaderModule(code);
    shaderModules[filename] = shaderModule;
}

VkShaderModule ShaderManager::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> ShaderManager::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}
