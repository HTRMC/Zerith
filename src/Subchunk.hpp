// #pragma once
// #include <random>
// #include <vector>
// #include <glm/glm.hpp>
// #include <glm/ext/matrix_transform.hpp>
//
// #include "BlockType.hpp"
// #include "CubeGeometry.hpp"
//
// class SubChunk {
// public:
//     static const int SIZE = 16;
//     static const int SUBCHUNK = SIZE * SIZE * SIZE;
//
//     SubChunk() {
//         generateRandomBlocks();
//         generateTransforms();
//     }
//
//     const std::vector<glm::mat4>& getTransforms() const {
//         return transforms;
//     }
//
//     const std::vector<Block>& getBlocks() const {
//         return blocks;
//     }
//
// private:
//     std::vector<glm::mat4> transforms;
//     std::vector<Block> blocks;
//
//     void generateRandomBlocks() {
//         blocks.resize(SUBCHUNK);
//
//         // Fill everything with stone by default
//         for (int i = 0; i < SUBCHUNK; i++) {
//             blocks[i].type = BlockType::STONE;
//         }
//
//         // Add dirt and grass layers
//         for (int x = 0; x < SIZE; x++) {
//             for (int y = 0; y < SIZE; y++) {
//                 // Top layer (grass)
//                 int topIndex = (15 * SIZE * SIZE) + (y * SIZE) + x;  // z = 15 (top layer)
//                 blocks[topIndex].type = BlockType::GRASS_BLOCK;
//
//                 // Second layer (dirt)
//                 int secondIndex = (14 * SIZE * SIZE) + (y * SIZE) + x;  // z = 14
//                 blocks[secondIndex].type = BlockType::DIRT;
//
//                 // Third layer (dirt)
//                 int thirdIndex = (13 * SIZE * SIZE) + (y * SIZE) + x;  // z = 13
//                 blocks[thirdIndex].type = BlockType::DIRT;
//             }
//         }
//     }
//
//     void generateTransforms() {
//         transforms.clear();
//         transforms.reserve(SUBCHUNK * 6); // 6 faces per cube
//
//         // Generate cube positions in a 16x16x16 grid
//         for (int x = 0; x < SIZE; x++) {
//             for (int y = 0; y < SIZE; y++) {
//                 for (int z = 0; z < SIZE; z++) {
//                     int blockIndex = (z * SIZE * SIZE) + (y * SIZE) + x;
//                     Block& block = blocks[blockIndex];
//
//                     if (block.type != BlockType::AIR) {
//                         glm::mat4 baseTransform = glm::translate(
//                             glm::mat4(1.0f),
//                             glm::vec3(static_cast<float>(x),
//                                      static_cast<float>(y),
//                                      static_cast<float>(z))
//                         );
//
//                         // Add transforms for each face of the cube
//                         for (int face = 0; face < 6; face++) {
//                             transforms.push_back(baseTransform * CUBE_FACE_TRANSFORMS[face]);
//                         }
//                     }
//                 }
//             }
//         }
//     }
// };
