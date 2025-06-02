#include "octree.h"
#include "logger.h"
#include "chunk.h"
#include <algorithm>
#include <functional>

namespace Zerith {

template <typename T>
Octree<T>::Octree(const AABB& bounds, int maxDepth, int maxObjectsPerNode)
    : maxDepth(maxDepth), maxObjectsPerNode(maxObjectsPerNode)
{
    root = std::make_unique<Node>();
    root->bounds = bounds;
}

template <typename T>
void Octree<T>::insert(const AABB& bounds, const T& object) {
    if (!root->bounds.contains(bounds)) {
        LOG_WARN("Octree: Object bounds outside of octree bounds");
        return;
    }
    
    insertInternal(root.get(), bounds, object, 0);
}

template <typename T>
void Octree<T>::insertInternal(Node* node, const AABB& bounds, const T& object, int depth) {
    // If we've reached maximum depth or this object should be inserted at this level, add it here
    if (depth >= maxDepth || shouldInsertAtThisLevel(node, bounds)) {
        node->objects.emplace_back(bounds, object);
        
        // Check if we need to subdivide based on object count
        if (node->isLeaf() && node->objects.size() > maxObjectsPerNode && depth < maxDepth) {
            subdivide(node);
            
            // Redistribute objects to children
            auto objectsToRedistribute = std::move(node->objects);
            for (const auto& [objBounds, obj] : objectsToRedistribute) {
                // Try to push down to children if possible
                bool inserted = false;
                for (int i = 0; i < CHILD_COUNT; i++) {
                    if (node->children[i] != nullptr && node->children[i]->bounds.contains(objBounds)) {
                        insertInternal(node->children[i].get(), objBounds, obj, depth + 1);
                        inserted = true;
                        break;
                    }
                }
                
                // If no child fully contains the object, keep it at this level
                if (!inserted) {
                    node->objects.emplace_back(objBounds, obj);
                }
            }
        }
        return;
    }
    
    // If node is a leaf but we're not at max depth, subdivide
    if (node->isLeaf()) {
        subdivide(node);
    }
    
    // Try to insert into a child that fully contains the object
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (node->children[i] != nullptr && node->children[i]->bounds.contains(bounds)) {
            insertInternal(node->children[i].get(), bounds, object, depth + 1);
            return;
        }
    }
    
    // If no child fully contains the object, add it to this node
    node->objects.emplace_back(bounds, object);
}

template <typename T>
bool Octree<T>::remove(const AABB& bounds, const T& object) {
    if (!root->bounds.intersects(bounds)) {
        return false;
    }
    
    return removeInternal(root.get(), bounds, object);
}

template <typename T>
bool Octree<T>::removeInternal(Node* node, const AABB& bounds, const T& object) {
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
    
    // Check children that might contain the object
    for (auto& child : node->children) {
        if (child != nullptr && child->bounds.intersects(bounds)) {
            if (removeInternal(child.get(), bounds, object)) {
                return true;
            }
        }
    }
    
    return false;
}

template <typename T>
bool Octree<T>::update(const AABB& oldBounds, const AABB& newBounds, const T& object) {
    // Remove and re-insert (could be optimized further)
    if (remove(oldBounds, object)) {
        insert(newBounds, object);
        return true;
    }
    return false;
}

template <typename T>
std::vector<std::pair<AABB, T>> Octree<T>::queryRegion(const AABB& region) const {
    std::vector<std::pair<AABB, T>> result;
    
    if (!root->bounds.intersects(region)) {
        return result;
    }
    
    queryRegionInternal(root.get(), region, result);
    return result;
}

template <typename T>
void Octree<T>::queryRegionInternal(const Node* node, const AABB& region, 
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
    for (const auto& child : node->children) {
        if (child != nullptr && child->bounds.intersects(region)) {
            queryRegionInternal(child.get(), region, result);
        }
    }
}

template <typename T>
std::vector<std::pair<AABB, T>> Octree<T>::queryRay(const glm::vec3& origin, 
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
void Octree<T>::queryRayInternal(const Node* node, const glm::vec3& origin, 
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
    
    for (int i = 0; i < CHILD_COUNT; i++) {
        if (!node->children[i]) continue;
        
        float t;
        if (node->children[i]->bounds.intersectsRay(origin, direction, t) && t <= maxDistance) {
            childrenDists.push_back({i, t});
        }
    }
    
    // Sort by distance
    std::sort(childrenDists.begin(), childrenDists.end());
    
    // Traverse children in order of distance
    for (const auto& cd : childrenDists) {
        queryRayInternal(node->children[cd.index].get(), origin, direction, maxDistance, result);
    }
}

template <typename T>
void Octree<T>::clear() {
    root = std::make_unique<Node>();
}

template <typename T>
void Octree<T>::subdivide(Node* node) {
    const glm::vec3 center = node->bounds.getCenter();
    const glm::vec3 extents = node->bounds.getExtents() * 0.5f;
    
    // Create 8 children nodes
    for (int i = 0; i < CHILD_COUNT; i++) {
        // Calculate child center based on octant
        glm::vec3 childCenter = center;
        childCenter.x += ((i & 1) ? extents.x : -extents.x);
        childCenter.y += ((i & 2) ? extents.y : -extents.y);
        childCenter.z += ((i & 4) ? extents.z : -extents.z);
        
        // Create child with appropriate bounds
        node->children[i] = std::make_unique<Node>();
        node->children[i]->bounds = AABB(childCenter - extents, childCenter + extents);
    }
}

template <typename T>
int Octree<T>::getChildIndex(const glm::vec3& nodeCenter, const glm::vec3& point) const {
    int index = 0;
    if (point.x >= nodeCenter.x) index |= 1;
    if (point.y >= nodeCenter.y) index |= 2;
    if (point.z >= nodeCenter.z) index |= 4;
    return index;
}

template <typename T>
bool Octree<T>::shouldInsertAtThisLevel(const Node* node, const AABB& bounds) const {
    // If bounds span multiple children, keep at this level
    const glm::vec3 center = node->bounds.getCenter();
    
    // Check if min and max points would go into different children
    int minIndex = getChildIndex(center, bounds.min);
    int maxIndex = getChildIndex(center, bounds.max);
    
    return minIndex != maxIndex;
}

// Explicit template instantiations for common types
// Add more as needed
template class Octree<int>;
template class Octree<float>;
template class Octree<glm::vec3>;
template class Octree<void*>;
template class Octree<Chunk*>;

} // namespace Zerith