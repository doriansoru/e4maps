#include "LayoutAlgorithm.hpp"
#include <cmath>
#include <algorithm>
#include <functional>

namespace LayoutAlgorithms {

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
        double baseRadius = 160.0;
        double radius = baseRadius * (1 + depth * 0.6); // Increase radius with depth to avoid overlap
        
        // Adjust radius based on number of children to prevent overlap
        if (node->children.size() > 0) {
            // Ensure adequate spacing between children
            double minSpacing = 100.0; // Minimum distance between child nodes
            double neededCircumference = node->children.size() * minSpacing;
            double minRadius = neededCircumference / (2 * M_PI);
            radius = std::max(radius, minRadius);
        }

        const double PI = 3.14159265359;
        double totalSector = endAngle - startAngle;
        if (node->isRoot()) totalSector = 2 * PI;

        // Distribute children more evenly based on their subtrees
        double anglePerChild = totalSector / node->children.size();
        double currentStart = (node->isRoot()) ? 0 : startAngle;

        for (auto& child : node->children) {
            double midAngle = currentStart + anglePerChild / 2.0;
            child->angle = midAngle;
            if (!child->manualPosition) {
                child->x = node->x + radius * std::cos(midAngle);
                child->y = node->y + radius * std::sin(midAngle);
            }
            
            // Recursively layout children with adjusted angular sectors
            calculateImprovedRadialLayout(child, 0, 0, currentStart, currentStart + anglePerChild, depth + 1);
            currentStart += anglePerChild;
        }
    }

    // Force-directed layout algorithm for better readability
    struct LayoutNode {
        std::shared_ptr<Node> node;
        double x, y;
        double fx, fy; // Forces
        bool fixed;

        LayoutNode(std::shared_ptr<Node> n, double initialX, double initialY) 
            : node(n), x(initialX), y(initialY), fx(0), fy(0), fixed(n->manualPosition) {}
    };

    void calculateForceDirectedLayout(std::shared_ptr<Node> root, int width, int height) {
        if (!root) return;
        
        // Collect all nodes for the force-directed algorithm
        std::vector<LayoutNode> layoutNodes;
        std::vector<std::pair<int, int>> edges; // connections between nodes
        
        // Helper function to traverse and collect nodes
        std::function<void(std::shared_ptr<Node>, int)> collectNodes = 
            [&](std::shared_ptr<Node> node, int parentId) {
                int currentIndex = layoutNodes.size();
                
                layoutNodes.emplace_back(
                    node, 
                    node->x,  // Use existing position as initial
                    node->y
                );
                
                // Add edges for connections
                for (auto& child : node->children) {
                    collectNodes(child, currentIndex);
                    edges.push_back({currentIndex, static_cast<int>(layoutNodes.size() - 1)});
                }
            };
        
        collectNodes(root, -1);

        if (layoutNodes.empty()) return;

        // Force-directed algorithm parameters
        const double k = 50.0; // Spring constant
        const double repulsion = 200.0; // Repulsion constant
        const double maxDisplacement = 50.0; // Max movement per iteration
        const int iterations = 50;

        for (int iter = 0; iter < iterations; iter++) {
            // Reset forces
            for (auto& ln : layoutNodes) {
                if (!ln.fixed) {
                    ln.fx = 0;
                    ln.fy = 0;
                }
            }

            // Calculate repulsive forces
            for (size_t i = 0; i < layoutNodes.size(); i++) {
                if (layoutNodes[i].fixed) continue;
                
                for (size_t j = 0; j < layoutNodes.size(); j++) {
                    if (i == j) continue;
                    
                    double dx = layoutNodes[i].x - layoutNodes[j].x;
                    double dy = layoutNodes[i].y - layoutNodes[j].y;
                    double distance = std::sqrt(dx * dx + dy * dy) + 0.1; // Avoid division by zero
                    
                    double force = repulsion / (distance * distance);
                    double fx = force * dx / distance;
                    double fy = force * dy / distance;
                    
                    layoutNodes[i].fx += fx;
                    layoutNodes[i].fy += fy;
                }
            }

            // Calculate attractive forces
            for (const auto& edge : edges) {
                auto& ln1 = layoutNodes[edge.first];
                auto& ln2 = layoutNodes[edge.second];
                
                if (ln1.fixed && ln2.fixed) continue; // If both are fixed, no spring force
                
                double dx = ln2.x - ln1.x;
                double dy = ln2.y - ln1.y;
                double distance = std::sqrt(dx * dx + dy * dy) + 0.1;
                
                double force = (distance * distance) / k; // Spring force
                double fx = force * dx / distance;
                double fy = force * dy / distance;
                
                if (!ln1.fixed) {
                    ln1.fx += fx;
                    ln1.fy += fy;
                }
                if (!ln2.fixed) {
                    ln2.fx -= fx;
                    ln2.fy -= fy;
                }
            }

            // Update positions
            for (auto& ln : layoutNodes) {
                if (ln.fixed) continue; // Don't move manually positioned nodes
                
                double displacement = std::sqrt(ln.fx * ln.fx + ln.fy * ln.fy);
                if (displacement > 0) {
                    double factor = std::min(maxDisplacement, displacement) / displacement;
                    ln.x += ln.fx * factor;
                    ln.y += ln.fy * factor;
                }
            }
        }

        // Update original nodes with new positions
        for (size_t i = 0; i < layoutNodes.size(); i++) {
            layoutNodes[i].node->x = layoutNodes[i].x;
            layoutNodes[i].node->y = layoutNodes[i].y;
        }
    }

} // namespace LayoutAlgorithms