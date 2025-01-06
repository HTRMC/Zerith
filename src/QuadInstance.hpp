#pragma once
#include <vector>
#include <cstdint>

// Instance data format:
// bits 0-2:   face type (3 bits)  -> 8 possible faces
// bits 3-7:   x position (5 bits) -> 32 positions
// bits 8-12:  y position (5 bits) -> 32 positions
// bits 13-17: z position (5 bits) -> 32 positions
// bits 18-31: reserved for future use

enum class FaceType {
    XNegative = 0,
    XPositive = 1,
    YNegative = 2,
    YPositive = 3,
    ZNegative = 4,
    ZPositive = 5
};

class QuadInstance {
public:
    static uint32_t createInstanceData(FaceType face, uint32_t x, uint32_t y, uint32_t z) {
        return (static_cast<uint32_t>(face) & 0x7) |
               ((x & 0x1F) << 3) |
               ((y & 0x1F) << 8) |
               ((z & 0x1F) << 13);
    }

    static std::vector<uint32_t> generateCubeInstance(uint32_t x, uint32_t y, uint32_t z) {
        std::vector<uint32_t> instances;
        instances.reserve(6);  // A cube has 6 faces

        // Add all six faces
        instances.push_back(createInstanceData(FaceType::XNegative, x, y, z)); // Left
        instances.push_back(createInstanceData(FaceType::XPositive, x, y, z)); // Right
        instances.push_back(createInstanceData(FaceType::YNegative, x, y, z)); // Back
        instances.push_back(createInstanceData(FaceType::YPositive, x, y, z)); // Front
        instances.push_back(createInstanceData(FaceType::ZNegative, x, y, z)); // Bottom
        instances.push_back(createInstanceData(FaceType::ZPositive, x, y, z)); // Top

        return instances;
    }

    // Generate a grid of cubes
    static std::vector<uint32_t> generateCubeGrid(uint32_t width, uint32_t height) {
        std::vector<uint32_t> instances;
        instances.reserve(width * height * 6); // 6 faces per cube

        for (uint32_t x = 0; x < width; x++) {
            for (uint32_t y = 0; y < height; y++) {
                auto cubeInstances = generateCubeInstance(x, y, 0);
                instances.insert(instances.end(), cubeInstances.begin(), cubeInstances.end());
            }
        }

        return instances;
    }

    static std::vector<uint32_t> generatePlaneInstances() {
        std::vector<uint32_t> instances;
        instances.reserve(16 * 16);  // 16x16 grid

        for (uint32_t x = 0; x < 16; x++) {
            for (uint32_t y = 0; y < 16; y++) {
                // For now, just create upward-facing quads (YPositive)
                instances.push_back(createInstanceData(FaceType::YPositive, x, 0, y));
            }
        }

        return instances;
    }
};
