#ifndef MINDMAP_DRAWER_HPP
#define MINDMAP_DRAWER_HPP

#include "MindMap.hpp"
#include "Theme.hpp"
#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <memory>
#include <vector>

class MindMapDrawer {
public:
    // Pre-calculate node dimensions to ensure arrows are positioned correctly
    void preCalculateNodeDimensions(std::shared_ptr<Node> node, const Theme& theme, const Cairo::RefPtr<Cairo::Context>& cr, int depth = 0);

    // Calculate node dimensions without drawing
    void calculateNodeDimensions(std::shared_ptr<Node> node, const Theme& theme, const Cairo::RefPtr<Cairo::Context>& cr, int depth);

    // Main draw function
    void drawNode(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, int depth, const Theme& theme, std::shared_ptr<Node> selectedNode = nullptr, const std::vector<std::shared_ptr<Node>>& selectedNodes = {}, const std::vector<Connection>& connections = {}, Connection* selectedConnection = nullptr, Connection* hoveredConnection = nullptr);

    // Draw arbitrary connections
    void drawArbitraryConnectionsForNode(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, const std::vector<Connection>& connections, const Theme& theme, int depth, Connection* selectedConnection = nullptr, Connection* hoveredConnection = nullptr);

    // Clear the image cache
    static void clearImageCache();

private:
    // Helper to load and cache images
    Glib::RefPtr<Gdk::Pixbuf> getCachedImage(const std::string& path, int reqW, int reqH);

    // Helper to draw a rounded rectangle
    void rounded_rectangle(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double width, double height, double radius);

    // Helper to calculate distance from center to rectangle boundary along an angle
    double getDistanceToRectBoundary(double width, double height, double angle);

    // Draw organic curved arrow connection
    void drawOrganicArrow(const Cairo::RefPtr<Cairo::Context>& cr,
                         double startX, double startY,
                         double endX, double endY,
                         double nodeWidth, double nodeHeight,
                         double width,
                         const Cairo::RefPtr<Cairo::Pattern>& color,
                         Color arrowColor,
                         int depth);

    void drawArrow(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double angle, double size, Color color);
};

#endif // MINDMAP_DRAWER_HPP