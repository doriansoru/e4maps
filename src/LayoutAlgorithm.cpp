#include "LayoutAlgorithm.hpp"
#include <cmath>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <vector>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace LayoutAlgorithms {

    // Helper to count leaves in a subtree for better angular distribution
    int countLeaves(std::shared_ptr<Node> node) {
        if (!node) return 0;
        if (node->children.empty()) return 1;
        int leaves = 0;
        for (auto& child : node->children) {
            leaves += countLeaves(child);
        }
        return leaves;
    }

    // Improved radial layout that spreads nodes more evenly
    void calculateImprovedRadialLayout(std::shared_ptr<Node> node, double cx, double cy,
                                       double startAngle, double endAngle, int depth) {
        if (!node) return;

        if (node->isRoot() && !node->manualPosition) {
            node->x = cx;
            node->y = cy;
        }
        if (node->children.empty()) return;

        // Calculate dynamic radius based on depth and number of children
        double baseRadius = 150.0; // Reduced base radius (was 200.0)
        double radius = baseRadius * (1 + depth * 0.4); 
        
        // Adjust radius based on child dimensions and count to prevent overlap
        double maxChildDim = 0;
        for (auto& child : node->children) {
            maxChildDim = std::max(maxChildDim, std::max(child->width, child->height));
        }
        if (maxChildDim < 50) maxChildDim = 120.0; // Default if not yet calculated

        double minSpacing = maxChildDim + 20.0; // Spacing based on node size (Reduced padding was 40.0)
        double neededCircumference = node->children.size() * minSpacing;
        double minRadius = neededCircumference / (std::abs(endAngle - startAngle) > 0.1 ? std::abs(endAngle - startAngle) : 2 * M_PI);
        radius = std::max(radius, minRadius);

        double totalSector = endAngle - startAngle;
        if (node->isRoot()) totalSector = 2 * M_PI;

        // Count total leaves in all children to weight angular sectors
        int totalLeaves = 0;
        std::vector<int> leafCounts;
        for (auto& child : node->children) {
            int c = countLeaves(child);
            leafCounts.push_back(c);
            totalLeaves += c;
        }

        double currentStart = (node->isRoot()) ? 0 : startAngle;

        for (size_t i = 0; i < node->children.size(); ++i) {
            auto& child = node->children[i];
            double childSector = totalSector * (static_cast<double>(leafCounts[i]) / totalLeaves);
            double midAngle = currentStart + childSector / 2.0;
            
            child->angle = midAngle;
            if (!child->manualPosition) {
                child->x = node->x + radius * std::cos(midAngle);
                child->y = node->y + radius * std::sin(midAngle);
            }
            
            // Recursively layout children with weighted angular sectors
            calculateImprovedRadialLayout(child, 0, 0, currentStart, currentStart + childSector, depth + 1);
            currentStart += childSector;
        }
    }

    // Helper for Tree Layout: Calculate total height of a subtree
    double getSubtreeHeight(std::shared_ptr<Node> node, double verticalSpacing) {
        if (!node || node->children.empty()) {
            return node ? node->height : 0.0;
        }
        
        double totalHeight = 0.0;
        for (auto& child : node->children) {
            totalHeight += getSubtreeHeight(child, verticalSpacing);
        }
        // Add spacing between children
        totalHeight += (node->children.size() - 1) * verticalSpacing;
        
        // The node itself might be taller than its children combined (unlikely but possible)
        return std::max(node->height, totalHeight);
    }

    // Helper for Tree Layout: Recursively position nodes
    void layoutSubtree(std::shared_ptr<Node> node, double x, double y, int direction, double horizontalSpacing, double verticalSpacing) {
        if (!node || node->children.empty()) return;
        
        double currentY = y - getSubtreeHeight(node, verticalSpacing) / 2.0;
        
        for (auto& child : node->children) {
            double childSubtreeHeight = getSubtreeHeight(child, verticalSpacing);
            double childY = currentY + childSubtreeHeight / 2.0;
            
            // Position child
            // Direction: 1 for Right, -1 for Left
            // We use parent's width and child's width to calculate gap
            double dist = (node->width / 2.0) + (child->width / 2.0) + horizontalSpacing;
            child->x = node->x + (direction * dist);
            child->y = childY;
            
            // Only update manualPosition if we want to enforce this layout strictly
            // child->manualPosition = true; // Optional: Force it
            
            layoutSubtree(child, child->x, child->y, direction, horizontalSpacing, verticalSpacing);
            
            currentY += childSubtreeHeight + verticalSpacing;
        }
    }

    void calculateTreeLayout(std::shared_ptr<Node> root) {
        if (!root) return;
        
        // Reset root to 0,0 if it's not manually positioned (or force it)
        if (!root->manualPosition) {
            root->x = 0;
            root->y = 0;
        }
        
        if (root->children.empty()) return;

        double horizontalSpacing = 50.0;
        double verticalSpacing = 20.0;

        // Split children into Left and Right lists
        std::vector<std::shared_ptr<Node>> leftChildren;
        std::vector<std::shared_ptr<Node>> rightChildren;
        
        // Simple alternating strategy for balance
        for (size_t i = 0; i < root->children.size(); ++i) {
            if (i % 2 == 0) {
                rightChildren.push_back(root->children[i]);
            } else {
                leftChildren.push_back(root->children[i]);
            }
        }
        
        // --- Process Right Side ---
        if (!rightChildren.empty()) {
            // Calculate total height of the right side to center it vertically relative to root
            double totalRightHeight = 0;
            for (auto& child : rightChildren) totalRightHeight += getSubtreeHeight(child, verticalSpacing);
            totalRightHeight += (rightChildren.size() - 1) * verticalSpacing;
            
            double currentY = root->y - totalRightHeight / 2.0;
            
            for (auto& child : rightChildren) {
                double h = getSubtreeHeight(child, verticalSpacing);
                double childY = currentY + h / 2.0;
                
                double dist = (root->width / 2.0) + (child->width / 2.0) + horizontalSpacing;
                child->x = root->x + dist;
                child->y = childY;
                
                layoutSubtree(child, child->x, child->y, 1, horizontalSpacing, verticalSpacing);
                
                currentY += h + verticalSpacing;
            }
        }
        
        // --- Process Left Side ---
        if (!leftChildren.empty()) {
            double totalLeftHeight = 0;
            for (auto& child : leftChildren) totalLeftHeight += getSubtreeHeight(child, verticalSpacing);
            totalLeftHeight += (leftChildren.size() - 1) * verticalSpacing;
            
            double currentY = root->y - totalLeftHeight / 2.0;
            
            for (auto& child : leftChildren) {
                double h = getSubtreeHeight(child, verticalSpacing);
                double childY = currentY + h / 2.0;
                
                double dist = (root->width / 2.0) + (child->width / 2.0) + horizontalSpacing;
                child->x = root->x - dist;
                child->y = childY;
                
                layoutSubtree(child, child->x, child->y, -1, horizontalSpacing, verticalSpacing);
                
                currentY += h + verticalSpacing;
            }
        }
    }

    // Force-directed layout algorithm for better readability
    struct LayoutNode {
        std::shared_ptr<Node> node;
        double x, y;
        double w, h; // Dimensions
        double fx, fy; // Forces
        bool fixed;

        LayoutNode(std::shared_ptr<Node> n, double initialX, double initialY) 
            : node(n), x(initialX), y(initialY), w(n->width), h(n->height), fx(0), fy(0), fixed(n->manualPosition) {
                // Reasonable defaults if dimensions aren't available
                if (w < 1.0) w = 120.0;
                if (h < 1.0) h = 30.0;
            }
    };

    void calculateForceDirectedLayout(std::shared_ptr<Node> root, int width, int height) {
        if (!root) return;
        
        std::vector<LayoutNode> layoutNodes;
        std::vector<std::pair<int, int>> edges;
        
        std::function<void(std::shared_ptr<Node>, int)> collectNodes = 
            [&](std::shared_ptr<Node> node, int parentId) {
                int currentIndex = layoutNodes.size();
                layoutNodes.emplace_back(node, node->x, node->y);
                if (node->isRoot()) layoutNodes.back().fixed = true;
                for (auto& child : node->children) {
                    collectNodes(child, currentIndex);
                    edges.push_back({currentIndex, static_cast<int>(layoutNodes.size() - 1)});
                }
            };
        
        collectNodes(root, -1);
        if (layoutNodes.empty()) return;

        // Fruchterman-Reingold inspired parameters for box-based layout
        const int iterations = 300; 
        double temp = 100.0; 
        const double coolingFactor = 0.98; 
        const double k = 100.0; // Balanced ideal distance (Reduced from 150.0)
        const double repulsionScale = 1.0; 

        // Deterministic RNG
        std::mt19937 gen(12345); // Fixed seed
        std::uniform_real_distribution<> dis(-0.5, 0.5);

        for (int iter = 0; iter < iterations; iter++) {
            // Reset forces
            for (auto& ln : layoutNodes) {
                ln.fx = 0; ln.fy = 0;
            }

            // 1. Repulsive Forces between ALL nodes
            for (size_t i = 0; i < layoutNodes.size(); i++) {
                for (size_t j = 0; j < layoutNodes.size(); j++) {
                    if (i == j) continue;

                    double dx = layoutNodes[i].x - layoutNodes[j].x;
                    double dy = layoutNodes[i].y - layoutNodes[j].y;
                    
                    // Handle perfect overlap by adding small random displacement
                    if (std::abs(dx) < 0.01 && std::abs(dy) < 0.01) {
                        dx = dis(gen);
                        dy = dis(gen);
                    }

                    double centerDist = std::sqrt(dx * dx + dy * dy) + 0.01;

                    // Calculate distance between boxes
                    const double minPadding = 15.0; // Slightly more padding (Reduced from 30.0)
                    double hDist = std::abs(dx) - (layoutNodes[i].w + layoutNodes[j].w) / 2.0 - minPadding;
                    double vDist = std::abs(dy) - (layoutNodes[i].h + layoutNodes[j].h) / 2.0 - minPadding;

                    double force;
                    if (hDist < 0 && vDist < 0) {
                        // OVERLAP! Strongest possible push
                        force = k * k * 10.0; 
                    } else {
                        // Distance is at least the separation between boxes or 1.0
                        double dist = std::max(1.0, std::max(hDist, vDist));
                        force = (k * k) / dist * repulsionScale;
                    }

                    layoutNodes[i].fx += (dx / centerDist) * force;
                    layoutNodes[i].fy += (dy / centerDist) * force;
                }
            }

            // 2. Attractive Forces along edges
            for (const auto& edge : edges) {
                auto& ln1 = layoutNodes[edge.first];
                auto& ln2 = layoutNodes[edge.second];
                
                double dx = ln2.x - ln1.x;
                double dy = ln2.y - ln1.y;
                double dist = std::sqrt(dx * dx + dy * dy) + 0.01;
                
                // Attraction force grows with distance
                double force = (dist * dist) / k; 
                
                double afx = (dx / dist) * force;
                double afy = (dy / dist) * force;

                if (!ln1.fixed) { ln1.fx += afx; ln1.fy += afy; }
                if (!ln2.fixed) { ln2.fx -= afx; ln2.fy -= afy; }
            }

            // 3. Limit displacement by temperature and apply
            for (auto& ln : layoutNodes) {
                if (ln.fixed) continue;

                double forceMag = std::sqrt(ln.fx * ln.fx + ln.fy * ln.fy) + 0.01;
                double scale = std::min(forceMag, temp) / forceMag;
                
                ln.x += ln.fx * scale;
                ln.y += ln.fy * scale;
            }

            temp *= coolingFactor; // Cool down
        }

        // Final pass: Update original nodes
        for (const auto& ln : layoutNodes) {
            ln.node->x = ln.x;
            ln.node->y = ln.y;
        }
    }

} // namespace LayoutAlgorithms
