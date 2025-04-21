#include "BlockState.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include "Logger.hpp"

// Static member initialization
std::mt19937 BlockState::rng;
bool BlockState::rngInitialized = false;

void BlockState::initRNG() {
    if (!rngInitialized) {
        // Seed the random number generator with current time
        unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
        rng.seed(seed);
        rngInitialized = true;
    }
}

bool BlockState::loadFromFile(const std::string& filename) {
    blockstatePath = filename;
    
    // Try to open the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open blockstate file: %s", filename.c_str());
        return false;
    }
    
    // Parse the JSON
    nlohmann::json json;
    try {
        file >> json;
        return parseJson(json);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse blockstate JSON from %s: %s", filename.c_str(), e.what());
        return false;
    }
}

bool BlockState::parseJson(const nlohmann::json& json) {
    // Clear existing variants
    variants.clear();
    
    try {
        bool foundValidVariants = false;

        // Check for the "multipart" format first (most complex)
        if (json.contains("multipart") && json["multipart"].is_array()) {
            foundValidVariants = parseMultipartFormat(json);
        }
        // Then check for the "variants" format
        else if (json.contains("variants") && json["variants"].is_object()) {
            foundValidVariants = parseVariantsFormat(json);
        }
        // Finally check for a simple single model blockstate
        else if (json.contains("model") && json["model"].is_string()) {
            foundValidVariants = parseSingleModelFormat(json);
        }

        if (foundValidVariants) {
            return true;
        }

        LOG_ERROR("No valid variants found in blockstate %s", blockstatePath.c_str());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error parsing blockstate %s: %s", blockstatePath.c_str(), e.what());
        return false;
    }
}

bool BlockState::parseMultipartFormat(const nlohmann::json& json) {
    auto multipartArray = json["multipart"];

    // If the multipart array is empty, there's nothing to parse
    if (multipartArray.empty()) {
        LOG_ERROR("Empty multipart array in blockstate %s", blockstatePath.c_str());
        return false;
    }

    // For multipart format, we'll extract the first basic model without conditions as our default
    bool foundDefaultModel = false;

    // First pass: look for entries without "when" conditions
    for (const auto& part : multipartArray) {
        // If there's no "when" condition, this is a default model that always applies
        if (!part.contains("when")) {
            // Check if it has an "apply" field
            if (part.contains("apply")) {
                // The "apply" field can be an object or an array of objects
                if (part["apply"].is_object()) {
                    // Single model
                    BlockVariant variant = parseApplyObject(part["apply"]);
                    if (!variant.modelPath.empty()) {
                        variant.isFromMultipart = true;
                        variants.push_back(variant);
                        foundDefaultModel = true;
                    }
                } else if (part["apply"].is_array()) {
                    // Multiple models - take the first valid one
                    for (const auto& applyJson : part["apply"]) {
                        BlockVariant variant = parseApplyObject(applyJson);
                        if (!variant.modelPath.empty()) {
                            variant.isFromMultipart = true;
                            variants.push_back(variant);
                            foundDefaultModel = true;
                            break; // Just take the first one
                        }
                    }
                }
            }
        }
    }

    // Second pass: If we didn't find a default model, look for entries with simple "when" conditions
    if (!foundDefaultModel) {
        for (const auto& part : multipartArray) {
            if (part.contains("when") && part.contains("apply")) {
                // Get the model(s) from the "apply" field
                std::vector<BlockVariant> partVariants;

                if (part["apply"].is_object()) {
                    // Single model
                    BlockVariant variant = parseApplyObject(part["apply"]);
                    if (!variant.modelPath.empty()) {
                        variant.isFromMultipart = true;
                        partVariants.push_back(variant);
                    }
                } else if (part["apply"].is_array()) {
                    // Multiple models - take all valid ones
                    for (const auto& applyJson : part["apply"]) {
                        BlockVariant variant = parseApplyObject(applyJson);
                        if (!variant.modelPath.empty()) {
                            variant.isFromMultipart = true;
                            partVariants.push_back(variant);
                        }
                    }
                }

                // If we found valid models, add the first one
                if (!partVariants.empty()) {
                    variants.push_back(partVariants[0]);
                    foundDefaultModel = true;
                    break;
                }
            }
        }
    }

    // Ensure we found at least one variant
    if (variants.empty()) {
        // As a last resort, create a fallback from the first part that has a model
        for (const auto& part : multipartArray) {
            if (part.contains("apply")) {
                if (part["apply"].is_object() && part["apply"].contains("model")) {
                    BlockVariant variant;
                    variant.modelPath = part["apply"]["model"].get<std::string>();

                    // Get optional rotations
                    if (part["apply"].contains("x") && part["apply"]["x"].is_number()) {
                        variant.rotationX = part["apply"]["x"].get<int>();
                    }

                    if (part["apply"].contains("y") && part["apply"]["y"].is_number()) {
                        variant.rotationY = part["apply"]["y"].get<int>();
                    }

                    // Get optional UV lock
                    if (part["apply"].contains("uvlock") && part["apply"]["uvlock"].is_boolean()) {
                        variant.uvlock = part["apply"]["uvlock"].get<bool>();
                    }

                    // Check if this is a mirrored model by looking at the path
                    if (variant.modelPath.find("_mirrored") != std::string::npos) {
                        variant.mirrored = true;
                    }

                    variant.isFromMultipart = true;
                    variants.push_back(variant);
                    foundDefaultModel = true;
                    break;
                } else if (part["apply"].is_array()) {
                    for (const auto& applyJson : part["apply"]) {
                        if (applyJson.contains("model")) {
                            BlockVariant variant;
                            variant.modelPath = applyJson["model"].get<std::string>();

                            // Get optional rotations
                            if (applyJson.contains("x") && applyJson["x"].is_number()) {
                                variant.rotationX = applyJson["x"].get<int>();
                            }

                            if (applyJson.contains("y") && applyJson["y"].is_number()) {
                                variant.rotationY = applyJson["y"].get<int>();
                            }

                            // Get optional UV lock
                            if (applyJson.contains("uvlock") && applyJson["uvlock"].is_boolean()) {
                                variant.uvlock = applyJson["uvlock"].get<bool>();
                            }

                            // Check if this is a mirrored model by looking at the path
                            if (variant.modelPath.find("_mirrored") != std::string::npos) {
                                variant.mirrored = true;
                            }

                            variant.isFromMultipart = true;
                            variants.push_back(variant);
                            foundDefaultModel = true;
                            break;
                        }
                    }
                    if (foundDefaultModel) break;
                }
            }
        }
    }

    // Store the multipart data for more advanced usage later
    multipartData = multipartArray;
    isMultipart = true;

    LOG_DEBUG("Loaded multipart blockstate %s with %zu variants", blockstatePath.c_str(), variants.size());
    return foundDefaultModel;
}

BlockVariant BlockState::parseApplyObject(const nlohmann::json& applyJson) {
    BlockVariant variant;

    // Get the model path
    if (applyJson.contains("model") && applyJson["model"].is_string()) {
        variant.modelPath = applyJson["model"].get<std::string>();
    } else {
        return variant; // Return empty variant if no model
    }

    // Get optional rotations
    if (applyJson.contains("x") && applyJson["x"].is_number()) {
        variant.rotationX = applyJson["x"].get<int>();
    }

    if (applyJson.contains("y") && applyJson["y"].is_number()) {
        variant.rotationY = applyJson["y"].get<int>();
    }

    // Get optional UV lock
    if (applyJson.contains("uvlock") && applyJson["uvlock"].is_boolean()) {
        variant.uvlock = applyJson["uvlock"].get<bool>();
    }

    // Check if this is a mirrored model by looking at the path
    if (variant.modelPath.find("_mirrored") != std::string::npos) {
        variant.mirrored = true;
    }

    return variant;
}

bool BlockState::parseVariantsFormat(const nlohmann::json& json) {
    auto variantsObj = json["variants"];
    bool foundVariants = false;

    // Check for various variant formats

    // Format 1: Direct "": { "model": "..." } format
    if (variantsObj.contains("") && variantsObj[""].is_object()) {
        auto variantJson = variantsObj[""];

        // Parse a single variant
        BlockVariant variant = parseVariantObject(variantJson);
        if (!variant.modelPath.empty()) {
            variants.push_back(variant);
            foundVariants = true;
            LOG_DEBUG("Loaded single default variant from blockstate %s", blockstatePath.c_str());
        }
    }
    // Format 2: Default variant as array: "": [ {...}, {...} ]
    else if (variantsObj.contains("") && variantsObj[""].is_array()) {
        auto variantsArray = variantsObj[""];

        // Process each variant in the array
        for (const auto& variantJson : variantsArray) {
            BlockVariant variant = parseVariantObject(variantJson);
            if (!variant.modelPath.empty()) {
                variants.push_back(variant);
                foundVariants = true;
            }
        }

        if (foundVariants) {
            LOG_DEBUG("Loaded %zu array variants from blockstate %s", variants.size(), blockstatePath.c_str());
        }
    }
    // Format 3: Property-based variants like "snowy=false": { ... }
    else {
        // Iterate through all properties
        for (auto it = variantsObj.begin(); it != variantsObj.end(); ++it) {
            std::string propertyKey = it.key();
            auto variantValue = it.value();

            // For each property, check if it's an object or array
            if (variantValue.is_object()) {
                // Single variant for this property
                BlockVariant variant = parseVariantObject(variantValue);
                if (!variant.modelPath.empty()) {
                    // Store the property in the variant
                    variant.property = propertyKey;
                    variants.push_back(variant);
                    foundVariants = true;
                }
            }
            else if (variantValue.is_array()) {
                // Multiple variants for this property
                for (const auto& variantJson : variantValue) {
                    BlockVariant variant = parseVariantObject(variantJson);
                    if (!variant.modelPath.empty()) {
                        // Store the property in the variant
                        variant.property = propertyKey;
                        variants.push_back(variant);
                        foundVariants = true;
                    }
                }
            }
        }

        if (foundVariants) {
            LOG_DEBUG("Loaded %zu property variants from blockstate %s", variants.size(), blockstatePath.c_str());
        }
    }

    return foundVariants;
}

bool BlockState::parseSingleModelFormat(const nlohmann::json& json) {
    BlockVariant blockVariant;
    blockVariant.modelPath = json["model"].get<std::string>();

    // Get optional rotations
    if (json.contains("x") && json["x"].is_number()) {
        blockVariant.rotationX = json["x"].get<int>();
    }

    if (json.contains("y") && json["y"].is_number()) {
        blockVariant.rotationY = json["y"].get<int>();
    }

    // Add the single variant
    variants.push_back(blockVariant);

    LOG_DEBUG("Loaded single variant from blockstate %s", blockstatePath.c_str());
    return true;
}

BlockVariant BlockState::parseVariantObject(const nlohmann::json& variantJson) {
    BlockVariant variant;

    // Get the model path
    if (variantJson.contains("model") && variantJson["model"].is_string()) {
        variant.modelPath = variantJson["model"].get<std::string>();
    } else {
        LOG_WARN("Variant in blockstate %s is missing model path", blockstatePath.c_str());
        return variant; // Return empty variant
    }

    // Get optional weight
    if (variantJson.contains("weight") && variantJson["weight"].is_number()) {
        variant.weight = variantJson["weight"].get<int>();
    }

    // Get optional rotations
    if (variantJson.contains("x") && variantJson["x"].is_number()) {
        variant.rotationX = variantJson["x"].get<int>();
    }

    if (variantJson.contains("y") && variantJson["y"].is_number()) {
        variant.rotationY = variantJson["y"].get<int>();
    }

    // Get optional UV lock
    if (variantJson.contains("uvlock") && variantJson["uvlock"].is_boolean()) {
        variant.uvlock = variantJson["uvlock"].get<bool>();
    }

    // Check if this is a mirrored model by looking at the path
    std::string modelPath = variant.modelPath;
    if (modelPath.find("_mirrored") != std::string::npos) {
        variant.mirrored = true;
    }

    return variant;
}

const BlockVariant& BlockState::getRandomVariant() const {
    if (variants.empty()) {
        // This should never happen if the blockstate was loaded correctly
        static BlockVariant defaultVariant;
        defaultVariant.modelPath = "minecraft:block/stone"; // Fallback
        LOG_ERROR("Attempted to get a variant from empty blockstate %s", blockstatePath.c_str());
        return defaultVariant;
    }
    
    // If there's only one variant, return it directly
    if (variants.size() == 1) {
        return variants[0];
    }
    
    // Initialize RNG if needed
    if (!rngInitialized) {
        const_cast<BlockState*>(this)->initRNG();
    }
    
    // Get all variants that match the default property (no property)
    std::vector<const BlockVariant*> matchingVariants;
    for (const auto& variant : variants) {
        if (variant.property.empty()) {
            matchingVariants.push_back(&variant);
        }
    }

    // If we found matching variants, use those - otherwise use all variants
    const std::vector<const BlockVariant*>& variantsToChooseFrom =
        matchingVariants.empty() ? std::vector<const BlockVariant*>{} : matchingVariants;

    // If we have no variants to choose from, use all variants
    if (variantsToChooseFrom.empty()) {
        // Calculate total weight
        int totalWeight = 0;
        for (const auto& variant : variants) {
            totalWeight += variant.weight;
        }

        // Generate a random number within the weight range
        std::uniform_int_distribution<int> dist(0, totalWeight - 1);
        int randomWeight = dist(rng);

        // Find the variant that corresponds to the random weight
        int currentWeight = 0;
        for (const auto& variant : variants) {
            currentWeight += variant.weight;
            if (randomWeight < currentWeight) {
                return variant;
            }
        }

        // Fallback to the last variant (shouldn't happen with proper weights)
        return variants.back();
    } else {
        // Calculate total weight for matching variants
        int totalWeight = 0;
        for (const auto* variant : variantsToChooseFrom) {
            totalWeight += variant->weight;
        }

        // Generate a random number within the weight range
        std::uniform_int_distribution<int> dist(0, totalWeight - 1);
        int randomWeight = dist(rng);

        // Find the variant that corresponds to the random weight
        int currentWeight = 0;
        for (const auto* variant : variantsToChooseFrom) {
            currentWeight += variant->weight;
            if (randomWeight < currentWeight) {
                return *variant;
            }
        }

        // Fallback to the last variant (shouldn't happen with proper weights)
        return *variantsToChooseFrom.back();
    }
}

const BlockVariant& BlockState::getVariant(size_t index) const {
    if (index < variants.size()) {
        return variants[index];
    }
    
    // If index is out of bounds, return the first variant or a default
    if (!variants.empty()) {
        return variants[0];
    }
    
    // This should never happen if the blockstate was loaded correctly
    static BlockVariant defaultVariant;
    defaultVariant.modelPath = "minecraft:block/stone"; // Fallback
    LOG_ERROR("Attempted to get a variant with invalid index %zu from blockstate %s", index, blockstatePath.c_str());
    return defaultVariant;
}

const BlockVariant& BlockState::getVariantForProperty(const std::string& propertyValue) const {
    // First look for exact property match
    for (const auto& variant : variants) {
        if (variant.property == propertyValue) {
            return variant;
        }
    }

    // If no exact match, return default variant (no property)
    for (const auto& variant : variants) {
        if (variant.property.empty()) {
            return variant;
        }
    }

    // If no default variant, return first variant
    if (!variants.empty()) {
        return variants[0];
    }

    // This should never happen if the blockstate was loaded correctly
    static BlockVariant defaultVariant;
    defaultVariant.modelPath = "minecraft:block/stone"; // Fallback
    LOG_ERROR("No variant found for property %s in blockstate %s", propertyValue.c_str(), blockstatePath.c_str());
    return defaultVariant;
}

bool BlockState::isMultipartState() const {
    return isMultipart;
}

const nlohmann::json& BlockState::getMultipartData() const {
    return multipartData;
}