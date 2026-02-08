#ifndef NODERENDERER_HPP
#define NODERENDERER_HPP

#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <memory>
#include "MindMap.hpp"
#include "Theme.hpp"

class NodeRenderer {
public:
    // Draw a single node (background, image, text)
    static void drawNodeContent(const Cairo::RefPtr<Cairo::Context>& cr, 
                                std::shared_ptr<Node> node, 
                                const Theme& theme, 
                                int depth, 
                                bool isSelected, 
                                bool isEditing);
    
    // Draw connection between two nodes
    static void drawConnection(const Cairo::RefPtr<Cairo::Context>& cr, 
                               std::shared_ptr<Node> from, 
                               std::shared_ptr<Node> to, 
                               const Theme& theme, 
                               int depth,
                               Cairo::RefPtr<Cairo::Pattern> colorPattern, 
                               E4Color arrowColor,
                               bool isSelected, 
                               bool isHovered);

    // Helper: Draw organic curved arrow
    static void drawOrganicArrow(const Cairo::RefPtr<Cairo::Context>& cr,
                                double startX, double startY,
                                double endX, double endY,
                                double nodeWidth, double nodeHeight,
                                double width,
                                const Cairo::RefPtr<Cairo::Pattern>& color,
                                E4Color arrowColor,
                                int depth);

    // Helper: Draw traditional arrow head
    static void drawArrowHead(const Cairo::RefPtr<Cairo::Context>& cr, 
                              double x, double y, double angle, double size, E4Color color);

    // Helper: Calculate distance from center to rectangle boundary along an angle
    static double getDistanceToRectBoundary(double width, double height, double angle);

    // Helper: Rounded Rectangle path
    static void roundedRectanglePath(const Cairo::RefPtr<Cairo::Context>& cr, 
                                     double x, double y, double width, double height, double radius);
};

#endif // NODERENDERER_HPP
