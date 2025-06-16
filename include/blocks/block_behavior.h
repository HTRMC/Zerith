#pragma once

#include "chunk.h"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>

namespace Zerith {

// Abstract base class for block behaviors
class BlockBehavior {
public:
    virtual ~BlockBehavior() = default;
    
    // Returns true if this block should provide collision (default: true)
    virtual bool hasCollision() const { return true; }
    
    // Returns true if this block can be walked through (default: false)
    virtual bool isWalkThrough() const { return false; }
    
    // Called when player enters this block (for special effects, etc.)
    virtual void onPlayerEnter(const glm::vec3& playerPos) {}
    
    // Called when player exits this block
    virtual void onPlayerExit(const glm::vec3& playerPos) {}
    
    // Returns true if this block should be rendered (default: true)
    virtual bool shouldRender() const { return true; }
};

// Registry for block behaviors
class BlockBehaviorRegistry {
public:
    static void registerBehavior(BlockType blockType, std::unique_ptr<BlockBehavior> behavior);
    static const BlockBehavior* getBehavior(BlockType blockType);
    static void initialize();
    
private:
    static std::unordered_map<BlockType, std::unique_ptr<BlockBehavior>> s_behaviors;
    static BlockBehavior s_defaultBehavior;
};

} // namespace Zerith