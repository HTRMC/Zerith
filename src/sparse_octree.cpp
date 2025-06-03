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
    root = std::make_unique<Node>();
    root->bounds = bounds;
}

template <typename T>
void SparseOctree<T>::insert(const AABB& bounds, const T& object) {
    if (!root->bounds.contains(bounds)) {
        LOG_WARN("SparseOctree: Object bounds outside of octree bounds");
        return;
    }
    
    insertInternal(root.get(), bounds, object, 0);
}

template <typename T>
void SparseOctree<T>::insertInternal(Node* node, const AABB& bounds, const T& object, int depth) {
    // If we've reached maximum depth or this object should be inserted at this level, add it here
    if (depth >= maxDepth || shouldInsertAtThisLevel(node, bounds)) {
        node->objects.emplace_back(bounds, object);
        
        // Check if we need to subdivide based on object count
        if (node->isLeaf() && node->objects.size() > maxObjectsPerNode && depth < maxDepth) {
            // Redistribute objects to children
            auto objectsToRedistribute = std::move(node->objects);
            for (const auto& [objBounds, obj] : objectsToRedistribute) {
                // Try to push down to appropriate child
                const glm::vec3 center = node->bounds.getCenter();
                int childIndex = getChildIndex(center, objBounds.getCenter());
                
                // If this object fits in a single child
                if (!shouldInsertAtThisLevel(node, objBounds)) {
                    // Create child if it doesn't exist
                    if (!node->hasChild(childIndex)) {
                        createChild(node, childIndex);
                    }
                    
                    // Insert into child
                    insertInternal(node->children[childIndex].get(), objBounds, obj, depth + 1);
                } else {
                    // If no child fully contains the object, keep it at this level
                    node->objects.emplace_back(objBounds, obj);
                }
            }
        }
        return;
    }
    
    // Calculate which child this object belongs to
    const glm::vec3 center = node->bounds.getCenter();
    int childIndex = getChildIndex(center, bounds.getCenter());
    
    // If object doesn't fit in a single child, add it to this node
    if (shouldInsertAtThisLevel(node, bounds)) {
        node->objects.emplace_back(bounds, object);
        return;
    }
    
    // Create child if it doesn't exist
    if (!node->hasChild(childIndex)) {
        createChild(node, childIndex);
    }
    
    // Insert into appropriate child
    insertInternal(node->children[childIndex].get(), bounds, object, depth + 1);
}

template <typename T>
bool SparseOctree<T>::remove(const AABB& bounds, const T& object) {
    if (!root->bounds.intersects(bounds)) {
        return false;
    }
    
    return removeInternal(root.get(), bounds, object);
}

template <typename T>
bool SparseOctree<T>::removeInternal(Node* node, const AABB& bounds, const T& object) {
    // Check this node's objects
    auto it = std::find_if(node->objects.begin(), node->objects.end(),
                         [&object](const auto& pair) { 
                             return pair.second == object; 
                         });
                         
    if (it != node->objects.end()) {
        node->objects.erase(it);
        return true;
    }
    
    // If this is a leaf node and we didn't find the object, it's not here
    if (node->isLeaf()) {
        return false;
    }
    
    // Try all children that might contain the object
    for (auto& [childIndex, child] : node->children) {
        if (child->bounds.intersects(bounds)) {
            if (removeInternal(child.get(), bounds, object)) {
                // Clean up empty child nodes
                if (child->isLeaf() && child->objects.empty()) {
                    node->children.erase(childIndex);
                }
                return true;
            }
        }
    }
    
    return false;
}

template <typename T>
bool SparseOctree<T>::update(const AABB& oldBounds, const AABB& newBounds, const T& object) {
    // Remove and re-insert (could be optimized further)
    if (remove(oldBounds, object)) {
        insert(newBounds, object);
        return true;
    }
    return false;
}

template <typename T>
std::vector<std::pair<AABB, T>> SparseOctree<T>::queryRegion(const AABB& region) const {
    std::vector<std::pair<AABB, T>> result;
    
    if (!root->bounds.intersects(region)) {
        return result;
    }
    
    queryRegionInternal(root.get(), region, result);
    return result;
}

template <typename T>
void SparseOctree<T>::queryRegionInternal(const Node* node, const AABB& region, 
                                       std::vector<std::pair<AABB, T>>& result) const {
    // Add any objects in this node that intersect with the query region
    for (const auto& [objBounds, obj] : node->objects) {
        if (objBounds.intersects(region)) {
            result.push_back(std::make_pair(objBounds, obj));
        }
    }
    
    // If leaf node, we're done
    if (node->isLeaf()) {
        return;
    }
    
    // Check children that intersect with the query region
    for (const auto& [childIndex, child] : node->children) {
        if (child->bounds.intersects(region)) {
            queryRegionInternal(child.get(), region, result);
        }
    }
}

template <typename T>
std::vector<std::pair<AABB, T>> SparseOctree<T>::queryRay(const glm::vec3& origin, 
                                                      const glm::vec3& direction, 
                                                      float maxDistance) const {
    std::vector<std::pair<AABB, T>> result;
    
    // Check if ray intersects the root node
    float t;
    if (!root->bounds.intersectsRay(origin, direction, t) || t > maxDistance) {
        return result;
    }
    
    queryRayInternal(root.get(), origin, direction, maxDistance, result);
    return result;
}

template <typename T>
void SparseOctree<T>::queryRayInternal(const Node* node, const glm::vec3& origin, 
                                    const glm::vec3& direction, float maxDistance,
                                    std::vector<std::pair<AABB, T>>& result) const {
    // Add any objects in this node that intersect with the ray
    for (const auto& [objBounds, obj] : node->objects) {
        float t;
        if (objBounds.intersectsRay(origin, direction, t) && t <= maxDistance) {
            result.push_back(std::make_pair(objBounds, obj));
        }
    }
    
    // If leaf node, we're done
    if (node->isLeaf()) {
        return;
    }
    
    // Sort children by distance from ray origin for more efficient traversal
    struct ChildDist {
        int index;
        float distance;
        bool operator<(const ChildDist& other) const {
            return distance < other.distance;
        }
    };
    
    std::vector<ChildDist> childrenDists;
    
    for (const auto& [childIndex, child] : node->children) {
        float t;
        if (child->bounds.intersectsRay(origin, direction, t) && t <= maxDistance) {
            childrenDists.push_back({childIndex, t});
        }
    }
    
    // Sort by distance
    std::sort(childrenDists.begin(), childrenDists.end());
    
    // Traverse children in order of distance
    for (const auto& cd : childrenDists) {
        queryRayInternal(node->children.at(cd.index).get(), origin, direction, maxDistance, result);
    }
}

template <typename T>
void SparseOctree<T>::clear() {
    root = std::make_unique<Node>();
}

template <typename T>
void SparseOctree<T>::createChild(Node* node, int childIndex) {
    node->children[childIndex] = std::make_unique<Node>();
    node->children[childIndex]->bounds = calculateChildBounds(node->bounds, childIndex);
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
    
    // Return child bounds
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
bool SparseOctree<T>::shouldInsertAtThisLevel(const Node* node, const AABB& bounds) const {
    // If bounds span multiple children, keep at this level
    const glm::vec3 center = node->bounds.getCenter();
    
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