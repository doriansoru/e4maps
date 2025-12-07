#ifndef LAYOUT_ALGORITHM_HPP
#define LAYOUT_ALGORITHM_HPP

#include "MindMap.hpp"
#include <vector>

namespace LayoutAlgorithms {

    // Improved radial layout that spreads nodes more evenly
    void calculateImprovedRadialLayout(std::shared_ptr<Node> node, double cx, double cy,
                                       double startAngle, double endAngle, int depth);

    // Force-directed layout algorithm for better readability
    void calculateForceDirectedLayout(std::shared_ptr<Node> root, int width, int height);

} // namespace LayoutAlgorithms

#endif // LAYOUT_ALGORITHM_HPP