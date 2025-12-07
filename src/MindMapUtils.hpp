#ifndef MIND_MAP_UTILS_HPP
#define MIND_MAP_UTILS_HPP

#include "MindMap.hpp"
#include "Constants.hpp"
#include <memory>
#include <algorithm>

namespace MindMapUtils {

    // Recursively calculates the bounding box of a node and its children.
    inline void calculateBoundsRecursive(std::shared_ptr<Node> node, double& minX, double& minY, double& maxX, double& maxY) {
        if (!node) return;

        // Update bounds based on node position
        minX = std::min(minX, node->x);
        minY = std::min(minY, node->y);
        maxX = std::max(maxX, node->x);
        maxY = std::max(maxY, node->y);

        // Check also for node dimensions (text and images)
        if (node->width > 0 || node->height > 0) {
            // Add node dimensions as padding
            double halfWidth = node->width / 2.0 + E4Maps::NODE_PADDING;
            double halfHeight = node->height / 2.0 + E4Maps::NODE_PADDING;

            minX = std::min(minX, node->x - halfWidth);
            minY = std::min(minY, node->y - halfHeight);
            maxX = std::max(maxX, node->x + halfWidth);
            maxY = std::max(maxY, node->y + halfHeight);
        }

        // Process all children
        for (auto& child : node->children) {
            calculateBoundsRecursive(child, minX, minY, maxX, maxY);
        }
    }

    // Calculates the bounding box of the entire mind map starting from the root node.
    // Returns true if bounds were calculated, false if the map or root is empty.
    inline bool calculateMapBounds(std::shared_ptr<Node> root_node, double& minX, double& minY, double& maxX, double& maxY) {
        if (!root_node) return false;

        // Initialize bounds with root node
        minX = root_node->x;
        minY = root_node->y;
        maxX = root_node->x;
        maxY = root_node->y;

        // Recursively find all node positions
        calculateBoundsRecursive(root_node, minX, minY, maxX, maxY);

        return true;
    }

} // namespace MindMapUtils

#endif // MIND_MAP_UTILS_HPP
