#include "sparse_octree.h"
#include "logger.h"
#include "chunk.h"
#include <algorithm>
#include <functional>

namespace Zerith {

template <typename T>
SparseOctree<T>::SparseOctree(const AABB& bounds, int maxDepth, int maxObjectsPerNode)
    : maxDepth(maxDepth), maxObjectsPerNode(maxObjectsPerNode)
{
    // Reserve some space to avoid frequent reallocations
    nodes.reserve(64);
    objects.reserve(128);
    
    // Create root node
    rootIndex = createNode(bounds);
}

template <typename T>
int SparseOctree<T>::createNode(const AABB& bounds) {
    int index = nodes.size();
    nodes.emplace_back();
    nodes[index].bounds = bounds;
    return index;
}

template <typename T>
uint32_t SparseOctree<T>::addObject(const AABB& bounds, const T& object) {
    uint32_t index = objects.size();
    objects.emplace_back(bounds, object);
    return index;
}

template <typename T>
void SparseOctree<T>::insert(const AABB& bounds, const T& object) {
    std::lock_guard<std::mutex> lock(m_octreeMutex);
    
    if (!nodes[rootIndex].bounds.contains(bounds)) {
        LOG_WARN("SparseOctree: Object bounds outside of octree bounds");
        return;
    }
    
    insertInternal(rootIndex, bounds, object, 0);
}

template <typename T>
void SparseOctree<T>::insertInternal(int nodeIndex, const AABB& bounds, const T& object, int depth) {
    // Safety check for debug mode - ensure nodeIndex is valid
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size())) {
        LOG_ERROR("SparseOctree: Invalid node index %d (max: %zu)", nodeIndex, nodes.size());
        return;
    }
    
    // Don't hold reference - access by index when needed

    // If we've reached maximum depth or this object should be inserted at this level, add it here
    if (depth >= maxDepth || shouldInsertAtThisLevel(nodeIndex, bounds)) {
        uint32_t objectIndex = addObject(bounds, object);
        nodes[nodeIndex].objectIndices.push_back(objectIndex);

        // Check if we need to subdivide based on object count
        if (nodes[nodeIndex].isLeaf() &&
            nodes[nodeIndex].objectIndices.size() > maxObjectsPerNode &&
            depth < maxDepth) {

            // Create children for node
            for (int i = 0; i < CHILD_COUNT; i++) {
                if (nodes[nodeIndex].childIndices[i] == -1) {
                    createChild(nodeIndex, i);
                }
            }

            // Access by index after potential reallocation
            // Move objects out before redistribution
            auto objectsToRedistribute = std::move(nodes[nodeIndex].objectIndices);
            nodes[nodeIndex].objectIndices.clear();

            for (uint32_t objIdx : objectsToRedistribute) {
                // Check object index is valid
                if (objIdx >= objects.size()) {
                    LOG_ERROR("SparseOctree: Invalid object index %u (max: %zu)", objIdx, objects.size());
                    continue;
                }

                const Object& obj = objects[objIdx];

                // Try to push down to appropriate child
                const glm::vec3 center = nodes[nodeIndex].bounds.getCenter();
                int childOctant = getChildIndex(center, obj.bounds.getCenter());

                // Validate childOctant
                if (childOctant < 0 || childOctant >= CHILD_COUNT) {
                    LOG_ERROR("SparseOctree: Invalid child octant %d", childOctant);
                    nodes[nodeIndex].objectIndices.push_back(objIdx);
                    continue;
                }

                if (nodes[nodeIndex].hasChild(childOctant)) {
                    int childIndex = nodes[nodeIndex].childIndices[childOctant];

                    // Validate child index
                    if (childIndex < 0 || childIndex >= static_cast<int>(nodes.size())) {
                        LOG_ERROR("SparseOctree: Invalid child node index %d (max: %zu)", childIndex, nodes.size());
                        nodes[nodeIndex].objectIndices.push_back(objIdx);
                        continue;
                    }

                    if (nodes[childIndex].bounds.contains(obj.bounds)) {
                        // Only recurse if the object can fully fit in the child node
                        insertInternal(childIndex, obj.bounds, obj.data, depth + 1);
                    } else {
                        // If can't push down, keep at this level
                        nodes[nodeIndex].objectIndices.push_back(objIdx);
                    }
                } else {
                    // If no child exists at this octant, keep at this level
                    nodes[nodeIndex].objectIndices.push_back(objIdx);
                }
            }
        }
        return;
    }

    // If node is a leaf but we're not at max depth, we need to create all children
    if (nodes[nodeIndex].isLeaf()) {
        for (int i = 0; i < CHILD_COUNT; i++) {
            createChild(nodeIndex, i);
        }
    }

    // Try to insert into appropriate child based on center point
    const glm::vec3 center = nodes[nodeIndex].bounds.getCenter();
    int childOctant = getChildIndex(center, bounds.getCenter());

    // Validate childOctant
    if (childOctant < 0 || childOctant >= CHILD_COUNT) {
        LOG_ERROR("SparseOctree: Invalid child octant %d", childOctant);
        uint32_t objectIndex = addObject(bounds, object);
        nodes[nodeIndex].objectIndices.push_back(objectIndex);
        return;
    }

    if (nodes[nodeIndex].hasChild(childOctant)) {
        int childIndex = nodes[nodeIndex].childIndices[childOctant];

        // Validate child index
        if (childIndex < 0 || childIndex >= static_cast<int>(nodes.size())) {
            LOG_ERROR("SparseOctree: Invalid child node index %d (max: %zu)", childIndex, nodes.size());
            uint32_t objectIndex = addObject(bounds, object);
            nodes[nodeIndex].objectIndices.push_back(objectIndex);
            return;
        }

        if (nodes[childIndex].bounds.contains(bounds)) {
            insertInternal(childIndex, bounds, object, depth + 1);
        } else {
            // If can't push down, add to this level
            uint32_t objectIndex = addObject(bounds, object);
            nodes[nodeIndex].objectIndices.push_back(objectIndex);
        }
    } else {
        // If can't push down, add to this level
        uint32_t objectIndex = addObject(bounds, object);
        nodes[nodeIndex].objectIndices.push_back(objectIndex);
    }
}

template <typename T>
bool SparseOctree<T>::remove(const AABB& bounds, const T& object) {
    std::lock_guard<std::mutex> lock(m_octreeMutex);
    
    if (!nodes[rootIndex].bounds.intersects(bounds)) {
        return false;
    }
    
    return removeInternal(rootIndex, bounds, object);
}

template <typename T>
bool SparseOctree<T>::removeInternal(int nodeIndex, const AABB& bounds, const T& object) {
    Node& node = nodes[nodeIndex];
    
    // Check this node's objects
    for (auto it = node.objectIndices.begin(); it != node.objectIndices.end(); ++it) {
        if (objects[*it].data == object) {
            node.objectIndices.erase(it);
            return true;
        }
    }
    
    // If this is a leaf node and we didn't find the object, it's not here
    if (node.isLeaf()) {
        return false;
    }
    
    // Check appropriate child first based on center point
    const glm::vec3 center = node.bounds.getCenter();
    int childOctant = getChildIndex(center, bounds.getCenter());
    
    if (node.hasChild(childOctant)) {
        int childIndex = node.childIndices[childOctant];
        if (nodes[childIndex].bounds.intersects(bounds) && 
            removeInternal(childIndex, bounds, object)) {
            return true;
        }
    }
    
    // Check other children if needed
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (i != childOctant && node.hasChild(i)) {
            int childIndex = node.childIndices[i];
            if (nodes[childIndex].bounds.intersects(bounds) && 
                removeInternal(childIndex, bounds, object)) {
                return true;
            }
        }
    }
    
    return false;
}

template <typename T>
bool SparseOctree<T>::update(const AABB& oldBounds, const AABB& newBounds, const T& object) {
    std::lock_guard<std::mutex> lock(m_octreeMutex);
    
    // Remove and re-insert (could be optimized further)
    if (removeInternal(rootIndex, oldBounds, object)) {
        insertInternal(rootIndex, newBounds, object, 0);
        return true;
    }
    return false;
}

template <typename T>
std::vector<std::pair<AABB, T>> SparseOctree<T>::queryRegion(const AABB& region) const {
    std::lock_guard<std::mutex> lock(m_octreeMutex);
    
    std::vector<std::pair<AABB, T>> result;
    
    if (!nodes[rootIndex].bounds.intersects(region)) {
        return result;
    }
    
    queryRegionInternal(rootIndex, region, result);
    return result;
}

template <typename T>
void SparseOctree<T>::queryRegionInternal(int nodeIndex, const AABB& region, 
                                   std::vector<std::pair<AABB, T>>& result) const {
    const Node& node = nodes[nodeIndex];
    
    // Add any objects in this node that intersect with the query region
    for (uint32_t objIndex : node.objectIndices) {
        const Object& obj = objects[objIndex];
        if (obj.bounds.intersects(region)) {
            result.emplace_back(obj.bounds, obj.data);
        }
    }
    
    // If leaf node, we're done
    if (node.isLeaf()) {
        return;
    }
    
    // Check children that intersect with the query region
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (node.hasChild(i)) {
            int childIndex = node.childIndices[i];
            if (nodes[childIndex].bounds.intersects(region)) {
                queryRegionInternal(childIndex, region, result);
            }
        }
    }
}

template <typename T>
std::vector<std::pair<AABB, T>> SparseOctree<T>::queryRay(const glm::vec3& origin, 
                                                  const glm::vec3& direction, 
                                                  float maxDistance) const {
    std::lock_guard<std::mutex> lock(m_octreeMutex);
    
    std::vector<std::pair<AABB, T>> result;
    
    // Check if ray intersects the root node
    float t;
    if (!nodes[rootIndex].bounds.intersectsRay(origin, direction, t) || t > maxDistance) {
        return result;
    }
    
    queryRayInternal(rootIndex, origin, direction, maxDistance, result);
    return result;
}

template <typename T>
void SparseOctree<T>::queryRayInternal(int nodeIndex, const glm::vec3& origin, 
                                const glm::vec3& direction, float maxDistance,
                                std::vector<std::pair<AABB, T>>& result) const {
    const Node& node = nodes[nodeIndex];
    
    // Add any objects in this node that intersect with the ray
    for (uint32_t objIndex : node.objectIndices) {
        const Object& obj = objects[objIndex];
        float t;
        if (obj.bounds.intersectsRay(origin, direction, t) && t <= maxDistance) {
            result.emplace_back(obj.bounds, obj.data);
        }
    }
    
    // If leaf node, we're done
    if (node.isLeaf()) {
        return;
    }
    
    // Use fixed-size array to avoid dynamic allocation and improve cache coherency
    struct ChildDist {
        int childIndex;
        float distance;
    };
    
    // Pre-allocated fixed-size array for better cache locality
    ChildDist childrenDists[CHILD_COUNT];
    int validChildCount = 0;
    
    // First, collect all valid children
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (node.hasChild(i)) {
            int childIndex = node.childIndices[i];
            float t;
            if (nodes[childIndex].bounds.intersectsRay(origin, direction, t) && t <= maxDistance) {
                childrenDists[validChildCount].childIndex = childIndex;
                childrenDists[validChildCount].distance = t;
                validChildCount++;
            }
        }
    }
    
    // Simple in-place insertion sort for small array (CHILD_COUNT is only 8)
    // This avoids the overhead of std::sort for very small arrays
    for (int i = 1; i < validChildCount; i++) {
        ChildDist key = childrenDists[i];
        int j = i - 1;
        
        while (j >= 0 && childrenDists[j].distance > key.distance) {
            childrenDists[j + 1] = childrenDists[j];
            j--;
        }
        
        childrenDists[j + 1] = key;
    }
    
    // Traverse children in order of distance
    for (int i = 0; i < validChildCount; i++) {
        queryRayInternal(childrenDists[i].childIndex, origin, direction, maxDistance, result);
    }
}

template <typename T>
void SparseOctree<T>::clear() {
    std::lock_guard<std::mutex> lock(m_octreeMutex);
    
    AABB rootBounds = nodes[rootIndex].bounds;
    
    nodes.clear();
    objects.clear();
    
    // Recreate the root node
    rootIndex = createNode(rootBounds);
}

template <typename T>
int SparseOctree<T>::createChild(int nodeIndex, int childOctant) {
    Node& node = nodes[nodeIndex];
    
    // Check if the child already exists
    if (node.hasChild(childOctant)) {
        return node.childIndices[childOctant];
    }
    
    // Calculate bounds for the child node
    AABB childBounds = calculateChildBounds(node.bounds, childOctant);
    
    // Create new node
    int childIndex = createNode(childBounds);
    
    // Update parent's child reference
    node.childIndices[childOctant] = childIndex;
    
    return childIndex;
}

template <typename T>
AABB SparseOctree<T>::calculateChildBounds(const AABB& parentBounds, int childIndex) const {
    const glm::vec3 center = parentBounds.getCenter();
    const glm::vec3 extents = parentBounds.getExtents() * 0.5f;
    
    // Calculate child center based on octant
    glm::vec3 childCenter = center;
    childCenter.x += ((childIndex & 1) ? extents.x : -extents.x);
    childCenter.y += ((childIndex & 2) ? extents.y : -extents.y);
    childCenter.z += ((childIndex & 4) ? extents.z : -extents.z);
    
    // Return AABB with min and max points
    return AABB(childCenter - extents, childCenter + extents);
}

template <typename T>
int SparseOctree<T>::getChildIndex(const glm::vec3& nodeCenter, const glm::vec3& point) const {
    int index = 0;
    if (point.x >= nodeCenter.x) index |= 1;
    if (point.y >= nodeCenter.y) index |= 2;
    if (point.z >= nodeCenter.z) index |= 4;
    return index;
}

template <typename T>
bool SparseOctree<T>::shouldInsertAtThisLevel(int nodeIndex, const AABB& bounds) const {
    const Node& node = nodes[nodeIndex];
    
    // If bounds span multiple children, keep at this level
    const glm::vec3 center = node.bounds.getCenter();
    
    // Check if min and max points would go into different children
    int minIndex = getChildIndex(center, bounds.min);
    int maxIndex = getChildIndex(center, bounds.max);
    
    return minIndex != maxIndex;
}

// Explicit template instantiations for common types
// Add more as needed
template class SparseOctree<int>;
template class SparseOctree<float>;
template class SparseOctree<glm::vec3>;
template class SparseOctree<void*>;
template class SparseOctree<Chunk*>;

} // namespace Zerith