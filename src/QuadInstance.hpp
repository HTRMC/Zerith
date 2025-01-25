#pragma once
#include <vector>
#include <cstdint>

// Instance data format:
// bits 0-2:   face type (3 bits)  -> 8 possible faces
// bits 3-6:   x position (4 bits) -> 16 positions
// bits 7-10:  y position (4 bits) -> 16 positions
// bits 11-14: z position (4 bits) -> 16 positions
// bits 15-18: width (4 bits)     -> Up to 16 blocks wide
// bits 19-22: height (4 bits)    -> Up to 16 blocks high
// bits 23-31: reserved

enum class FaceType {
    XNegative = 0,
    XPositive = 1,
    YNegative = 2,
    YPositive = 3,
    ZNegative = 4,
    ZPositive = 5
};
