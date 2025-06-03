#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>
#include "aabb.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace Zerith {

template <typename T>
class SparseOctree {
public:
    static constexpr int CHILD_COUNT = 8;
    
    struct Node {
        AABB bounds;
        std::unordered_map<int, std::unique_ptr<Node>> children;
        std::vector<std::pair<AABB, T>> objects;
        
        bool isLeaf() const { return children.empty(); }
        bool hasChild(int index) const { return children.find(index) != children.end(); }
    };

    SparseOctree(const AABB& bounds, int maxDepth = 8, int maxObjectsPerNode = 16);
    ~SparseOctree() = default;

    // Insert an object with its bounding box
    void insert(const AABB& bounds, const T& object);
    
    // Remove an object
    bool remove(const AABB& bounds, const T& object);
    
    // Update an object's position
    bool update(const AABB& oldBounds, const AABB& newBounds, const T& object);
    
    // Query objects within a region
    std::vector<std::pair<AABB, T>> queryRegion(const AABB& region) const;
    
    // Query objects that intersect with a ray
    std::vector<std::pair<AABB, T>> queryRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = std::numeric_limits<float>::max()) const;
    
    // Clear the entire octree
    void clear();

    // Get the root node for advanced operations
    const Node* getRoot() const { return root.get(); }

private:
    std::unique_ptr<Node> root;
    int maxDepth;
    int maxObjectsPerNode;
    
    // Helper methods
    void insertInternal(Node* node, const AABB& bounds, const T& object, int depth);
    bool removeInternal(Node* node, const AABB& bounds, const T& object);
    void queryRegionInternal(const Node* node, const AABB& region, std::vector<std::pair<AABB, T>>& result) const;
    void queryRayInternal(const Node* node, const glm::vec3& origin, const glm::vec3& direction, 
                         float maxDistance, std::vector<std::pair<AABB, T>>& result) const;
    
    // Create a child for a node at specified index
    void createChild(Node* node, int childIndex);
    
    // Calculate child bounds based on parent and child index
    AABB calculateChildBounds(const AABB& parentBounds, int childIndex) const;
    
    // Determine which child index a point belongs to
    int getChildIndex(const glm::vec3& center, const glm::vec3& point) const;
    
    // Check if object should be inserted at this level based on bounds
    bool shouldInsertAtThisLevel(const Node* node, const AABB& bounds) const;
};

} // namespace Zerith