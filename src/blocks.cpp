#include "blocks.h"
#include "blocks/fluid_block.h"
#include "logger.h"

namespace Zerith
{
    // Static members
    std::vector<BlockDefPtr> Blocks::blocks_;
    std::unordered_map<std::string, size_t> Blocks::idToIndex_;
    bool Blocks::initialized_ = false;

    // Block type constants
    BlockType Blocks::AIR = 0;
    BlockType Blocks::STONE = 0;
    BlockType Blocks::DIRT = 0;
    BlockType Blocks::GRASS_BLOCK = 0;
    BlockType Blocks::COBBLESTONE = 0;
    BlockType Blocks::SAND = 0;
    BlockType Blocks::GRAVEL = 0;
    BlockType Blocks::OAK_LOG = 0;
    BlockType Blocks::OAK_PLANKS = 0;
    BlockType Blocks::OAK_LEAVES = 0;
    BlockType Blocks::OAK_SLAB = 0;
    BlockType Blocks::OAK_STAIRS = 0;
    BlockType Blocks::STONE_BRICKS = 0;
    BlockType Blocks::BRICKS = 0;
    BlockType Blocks::COAL_ORE = 0;
    BlockType Blocks::IRON_ORE = 0;
    BlockType Blocks::DIAMOND_ORE = 0;
    BlockType Blocks::GLASS = 0;
    BlockType Blocks::GLOWSTONE = 0;
    BlockType Blocks::WATER = 0;
    BlockType Blocks::CRAFTING_TABLE = 0;

    BlockType Blocks::registerBlock(const std::string& id, const BlockSettings& settings,
                                    std::unique_ptr<BlockBehavior> behavior)
    {
        auto block = registerBlockInternal(id, settings, std::move(behavior));
        return block->getBlockType();
    }

    BlockDefPtr Blocks::registerBlockInternal(const std::string& id, const BlockSettings& settings,
                                              std::unique_ptr<BlockBehavior> behavior)
    {
        size_t index = blocks_.size();

        auto block = std::make_shared<BlockDefinition>(id, settings, std::move(behavior));
        block->setBlockType(static_cast<BlockType>(index));

        blocks_.push_back(block);
        idToIndex_[id] = index;

        LOG_DEBUG("Registered block '%s' with type %d", id.c_str(), static_cast<int>(index));

        return block;
    }

    BlockDefPtr Blocks::getBlock(BlockType type)
    {
        size_t index = static_cast<size_t>(type);
        if (index < blocks_.size())
        {
            return blocks_[index];
        }
        return nullptr;
    }

    BlockDefPtr Blocks::getBlock(const std::string& id)
    {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end())
        {
            return blocks_[it->second];
        }
        return nullptr;
    }

    RenderLayer Blocks::getRenderLayer(BlockType type)
    {
        auto block = getBlock(type);
        return block ? block->getRenderLayer() : RenderLayer::OPAQUE;
    }

    const BlockBehavior* Blocks::getBehavior(BlockType type)
    {
        auto block = getBlock(type);
        return block ? block->getBehavior() : nullptr;
    }

    bool Blocks::hasCollision(BlockType type)
    {
        auto block = getBlock(type);
        return block ? block->hasCollision() : true; // Default to collision
    }

    bool Blocks::hasCollisionAt(BlockType type, const glm::vec3& position, const AABB& entityAABB)
    {
        auto block = getBlock(type);
        return block ? block->hasCollisionAt(position, entityAABB) : true; // Default to collision
    }

    void Blocks::initialize()
    {
        if (initialized_)
        {
            LOG_WARN("Blocks already initialized, skipping");
            return;
        }

        LOG_INFO("Initializing blocks system...");

        AIR = registerBlock("air", BlockSettings::create().material(BlockMaterial::AIR));
        STONE = registerBlock("stone", BlockSettings::create().material(BlockMaterial::STONE));
        DIRT = registerBlock("dirt", BlockSettings::create().material(BlockMaterial::SOLID));
        GRASS_BLOCK = registerBlock("grass_block", BlockSettings::create().material(BlockMaterial::SOLID));
        COBBLESTONE = registerBlock("cobblestone", BlockSettings::create().material(BlockMaterial::STONE));
        SAND = registerBlock("sand", BlockSettings::create().material(BlockMaterial::SOLID));
        GRAVEL = registerBlock("gravel", BlockSettings::create().material(BlockMaterial::SOLID));
        OAK_LOG = registerBlock("oak_log", BlockSettings::create().material(BlockMaterial::WOOD));
        OAK_PLANKS = registerBlock("oak_planks", BlockSettings::create().material(BlockMaterial::WOOD));
        OAK_LEAVES = registerBlock("oak_leaves", BlockSettings::create().material(BlockMaterial::LEAVES));
        OAK_SLAB = registerBlock("oak_slab", BlockSettings::create().material(BlockMaterial::WOOD).slab());
        OAK_STAIRS = registerBlock("oak_stairs", BlockSettings::create().material(BlockMaterial::WOOD).stairs());
        STONE_BRICKS = registerBlock("stone_bricks", BlockSettings::create().material(BlockMaterial::STONE));
        BRICKS = registerBlock("bricks", BlockSettings::create().material(BlockMaterial::STONE));
        COAL_ORE = registerBlock("coal_ore", BlockSettings::create().material(BlockMaterial::STONE));
        IRON_ORE = registerBlock("iron_ore", BlockSettings::create().material(BlockMaterial::STONE));
        DIAMOND_ORE = registerBlock("diamond_ore", BlockSettings::create().material(BlockMaterial::STONE));
        GLASS = registerBlock("glass", BlockSettings::create().material(BlockMaterial::GLASS));
        GLOWSTONE = registerBlock("glowstone", BlockSettings::create().material(BlockMaterial::SOLID).transparent());
        CRAFTING_TABLE = registerBlock("crafting_table", BlockSettings::create().material(BlockMaterial::WOOD));

        // Water with custom fluid behavior
        auto waterSettings = BlockSettings::create().material(BlockMaterial::LIQUID);
        auto waterBehavior = std::make_unique<FluidBlock>(waterSettings);
        WATER = registerBlock("water", waterSettings, std::move(waterBehavior));

        initialized_ = true;
        LOG_INFO("Blocks system initialized with %zu blocks", blocks_.size());
    }
} // namespace Zerith
