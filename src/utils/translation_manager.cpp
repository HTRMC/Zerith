#include "translation_manager.h"
#include "logger.h"
#include <fstream>
#include <sstream>

namespace Zerith {

TranslationManager* TranslationManager::instance_ = nullptr;

void TranslationManager::loadLanguageFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open language file: %s", filepath.c_str());
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Parse JSON content using simple string parsing
    size_t pos = 0;
    while (true) {
        // Find next key
        size_t keyStart = content.find("\"", pos);
        if (keyStart == std::string::npos) break;
        keyStart++; // Skip opening quote
        
        size_t keyEnd = content.find("\"", keyStart);
        if (keyEnd == std::string::npos) break;
        
        std::string key = content.substr(keyStart, keyEnd - keyStart);
        
        // Skip to colon
        size_t colonPos = content.find(":", keyEnd);
        if (colonPos == std::string::npos) break;
        
        // Find value
        size_t valueStart = content.find("\"", colonPos);
        if (valueStart == std::string::npos) break;
        valueStart++; // Skip opening quote
        
        size_t valueEnd = content.find("\"", valueStart);
        if (valueEnd == std::string::npos) break;
        
        std::string value = content.substr(valueStart, valueEnd - valueStart);
        
        // Store translation
        translations_[key] = value;
        
        // Move to next entry
        pos = valueEnd + 1;
        
        // Skip to next key or end
        size_t commaPos = content.find(",", pos);
        if (commaPos == std::string::npos) break;
        pos = commaPos + 1;
    }
    
    LOG_INFO("Loaded %zu translations from %s", translations_.size(), filepath.c_str());
}

std::string TranslationManager::translate(const std::string& key) const {
    auto it = translations_.find(key);
    if (it != translations_.end()) {
        return it->second;
    }
    
    // Return the key if no translation found
    return key;
}

} // namespace Zerith