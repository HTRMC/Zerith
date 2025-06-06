#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>
#include <mutex>
#include "aabb.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace Zerith {

template <typename T>
class SparseOctree {
public:
    static constexpr int CHILD_COUNT = 8;
    
    struct Object {
        AABB bounds;
        T data;
        
        Object(const AABB& b, const T& d) : bounds(b), data(d) {}
        bool operator==(const Object& other) const { return data == other.data; }
    };

    struct Node {
        AABB bounds;
        // Using a fixed array to store indices of children (or -1 if no child)
        // This avoids the cache-unfriendly unordered_map with pointer indirection
        std::array<int, CHILD_COUNT> childIndices;
        // Store object indices instead of objects themselves
        std::vector<uint32_t> objectIndices;
        
        Node() : childIndices{-1, -1, -1, -1, -1, -1, -1, -1} {}
        
        bool isLeaf() const { 
            return std::all_of(childIndices.begin(), childIndices.end(), 
                              [](int idx) { return idx == -1; }); 
        }
        
        bool hasChild(int index) const { 
            return index >= 0 && index < CHILD_COUNT && childIndices[index] != -1; 
        }
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
    const Node& getRoot() const { return nodes[rootIndex]; }

private:
    // Mutex for thread safety
    mutable std::mutex m_octreeMutex;
    
    // Store all nodes in a contiguous vector for better cache locality
    std::vector<Node> nodes;
    // Store all objects in a contiguous vector
    std::vector<Object> objects;
    // Root node is always at index 0
    int rootIndex;
    int maxDepth;
    int maxObjectsPerNode;
    
    // Helper methods
    void insertInternal(int nodeIndex, const AABB& bounds, const T& object, int depth);
    bool removeInternal(int nodeIndex, const AABB& bounds, const T& object);
    void queryRegionInternal(int nodeIndex, const AABB& region, std::vector<std::pair<AABB, T>>& result) const;
    void queryRayInternal(int nodeIndex, const glm::vec3& origin, const glm::vec3& direction, 
                         float maxDistance, std::vector<std::pair<AABB, T>>& result) const;
    
    // Create a child for a node at specified index
    int createChild(int nodeIndex, int childOctant);
    
    // Calculate child bounds based on parent and child index
    AABB calculateChildBounds(const AABB& parentBounds, int childIndex) const;
    
    // Determine which child index a point belongs to
    int getChildIndex(const glm::vec3& center, const glm::vec3& point) const;
    
    // Check if object should be inserted at this level based on bounds
    bool shouldInsertAtThisLevel(int nodeIndex, const AABB& bounds) const;
    
    // Create a new node and return its index
    int createNode(const AABB& bounds);
    
    // Add a new object and return its index
    uint32_t addObject(const AABB& bounds, const T& object);
};

} // namespace Zerith