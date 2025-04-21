#pragma once

#include <string>
#include <vector>
#include <random>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

// Forward declarations
class ModelLoader;

// Represents a single variant of a block model with rotation/transformation
struct BlockVariant {
    std::string modelPath;   // Path to the model JSON
    int weight = 1;          // Weight for random selection (higher = more likely)
    int rotationX = 0;       // X rotation in degrees (0, 90, 180, 270)
    int rotationY = 0;       // Y rotation in degrees (0, 90, 180, 270)
    bool uvlock = false;     // Whether to lock UV coordinates during rotation
    bool mirrored = false;   // Whether the model is mirrored (x-axis flipped)
    std::string property;    // Property value this variant corresponds to (e.g., "snowy=false")
    bool isFromMultipart = false; // Whether this variant comes from a multipart blockstate
};

// Represents a blockstate, which may have multiple variants
class BlockState {
public:
    BlockState() = default;
    ~BlockState() = default;

    // Load a blockstate from a file
    bool loadFromFile(const std::string& filename);

    // Parse blockstate JSON data
    bool parseJson(const nlohmann::json& json);

    // Parse the different blockstate formats
    bool parseMultipartFormat(const nlohmann::json& json);
    bool parseVariantsFormat(const nlohmann::json& json);
    bool parseSingleModelFormat(const nlohmann::json& json);

    // Parse a single variant object from JSON
    BlockVariant parseVariantObject(const nlohmann::json& variantJson);

    // Parse an "apply" object from multipart format
    BlockVariant parseApplyObject(const nlohmann::json& applyJson);

    // Get a random variant based on weights
    const BlockVariant& getRandomVariant() const;

    // Get a specific variant by index
    const BlockVariant& getVariant(size_t index) const;

    // Get a variant for a specific property value (e.g., "snowy=false")
    const BlockVariant& getVariantForProperty(const std::string& propertyValue) const;

    // Get the number of variants
    size_t getVariantCount() const { return variants.size(); }

    // Get the blockstate path
    const std::string& getPath() const { return blockstatePath; }

    // Check if this is a multipart blockstate
    bool isMultipartState() const;

    // Get the raw multipart data
    const nlohmann::json& getMultipartData() const;

private:
    std::string blockstatePath;
    std::vector<BlockVariant> variants;
    bool isMultipart = false;
    nlohmann::json multipartData;

    // Random number generator for variant selection
    static std::mt19937 rng;
    static bool rngInitialized;

    // Initialize RNG if needed
    static void initRNG();
};