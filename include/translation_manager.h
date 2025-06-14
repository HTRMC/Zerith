#pragma once

#include <string>
#include <unordered_map>

namespace Zerith {

class TranslationManager {
private:
    std::unordered_map<std::string, std::string> translations_;
    static TranslationManager* instance_;
    
    TranslationManager() = default;

public:
    static TranslationManager& getInstance() {
        if (!instance_) {
            instance_ = new TranslationManager();
        }
        return *instance_;
    }
    
    void loadLanguageFile(const std::string& filepath);
    
    std::string translate(const std::string& key) const;
    
    static std::string getTranslationKey(const std::string& blockId) {
        return "block.zerith." + blockId;
    }
};

} // namespace Zerith