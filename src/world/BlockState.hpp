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

    // Support for additional transformations can be added here
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

    // Parse a single variant object from JSON
    BlockVariant parseVariantObject(const nlohmann::json& variantJson);

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

private:
    std::string blockstatePath;
    std::vector<BlockVariant> variants;

    // Random number generator for variant selection
    static std::mt19937 rng;
    static bool rngInitialized;

    // Initialize RNG if needed
    static void initRNG();
};