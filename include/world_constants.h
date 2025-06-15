#pragma once

namespace Zerith {

// World height constants matching Minecraft 1.18+
constexpr int WORLD_MIN_Y = -64;
constexpr int WORLD_MAX_Y = 319;
constexpr int WORLD_HEIGHT = WORLD_MAX_Y - WORLD_MIN_Y + 1; // 384 blocks total
constexpr int SEA_LEVEL = 62;

// Chunk organization in Y-axis
constexpr int CHUNKS_Y = 24; // 384 / 16 = 24 chunks vertically

} // namespace Zerith