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

    // Recursively resets manualPosition flag for all nodes.
    inline void resetManualPositionsRecursive(std::shared_ptr<Node> node) {
        if (!node) return;
        node->manualPosition = false;
        for (auto& child : node->children) {
            resetManualPositionsRecursive(child);
        }
    }

    /**
     * Calculates the control point for the organic curved arrows used in connections.
     * Centralizing this logic ensures consistency between hit testing (selection) and drawing.
     */
    inline void calculateOrganicBezierControlPoint(double startX, double startY, double endX, double endY, 
                                                  int depth, double& ctrlX, double& ctrlY) {
        double dx = endX - startX;
        double dy = endY - startY;
        double distance = std::sqrt(dx * dx + dy * dy);

        if (distance < 0.1) {
            ctrlX = startX;
            ctrlY = startY;
            return;
        }

        double midX = (startX + endX) / 2.0;
        double midY = (startY + endY) / 2.0;
        double perpX = -dy / distance;
        double perpY = dx / distance;

        // Curve offset logic: reduce curve as depth increases
        double curveOffset = (distance / 4.0) * (1.0 - (depth * 0.1));

        // Use a deterministic pseudo-random based on coordinates to maintain consistency
        unsigned int seed = (unsigned int)((startX + startY + endX + endY) * 1000);
        double rand_offset = ((seed % 1000) / 1000.0 - 0.5) * 0.3;
        curveOffset *= (1.0 + rand_offset);

        ctrlX = midX + perpX * curveOffset;
        ctrlY = midY + perpY * curveOffset;
    }

} // namespace MindMapUtils

#endif // MIND_MAP_UTILS_HPP
