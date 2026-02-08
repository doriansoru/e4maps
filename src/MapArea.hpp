#ifndef MAPAREA_HPP
#define MAPAREA_HPP

#include <gtkmm.h>
#include <memory>
#include "DrawingContext.hpp"

// Forward declarations
class MindMap;
class Node;

class MapArea : public Gtk::DrawingArea {
    DrawingContext drawingContext;

    bool isDragging = false;
    bool isPanning = false;
    bool isPreDragging = false;  // Flag to indicate potential dragging (for threshold detection)
    double dragStartX, dragStartY;
    double panStartOffsetX, panStartOffsetY;
    double nodeStartX, nodeStartY;
    // Track previous mouse position for smooth incremental movement
    double prevMouseWorldX, prevMouseWorldY;
    bool isFirstDragMotion = true;

    // Track initial positions for Undo support
    std::vector<std::shared_ptr<Node>> initialDragNodes;
    std::vector<std::pair<double, double>> initialDragPositions;

public:
    sigc::signal<void, std::shared_ptr<Node>> signal_edit_node;
    sigc::signal<void, GdkEventButton*, std::shared_ptr<Node>> signal_node_context_menu;
    // New signal for connection context menu
    sigc::signal<void, GdkEventButton*, std::shared_ptr<Connection>> signal_connection_context_menu; 
    sigc::signal<void> signal_map_modified;
    // New signal for drag completion
    sigc::signal<void, const std::vector<std::shared_ptr<Node>>&, const std::vector<std::pair<double, double>>&, const std::vector<std::pair<double, double>>&> signal_nodes_moved;
    sigc::signal<void> signal_selection_changed;

    explicit MapArea(std::shared_ptr<MindMap> m);
    ~MapArea() override = default;

    std::shared_ptr<Node> getSelectedNode() const { return drawingContext.getSelectedNode(); }

    // Returns currently selected connection (nullptr if none)
    std::shared_ptr<Connection> getSelectedConnection() const { return drawingContext.getSelectedConnection(); }

    const std::vector<std::shared_ptr<Node>>& getSelectedNodes() const { return drawingContext.getSelectedNodes(); }

    void setMap(std::shared_ptr<MindMap> m);

    void setSelectedNodes(const std::vector<std::shared_ptr<Node>>& nodes);
    
    // Wrapper for clearing connection selection
    void clearConnectionSelection() { drawingContext.clearConnectionSelection(); }
    
    double getScale() const { return drawingContext.getViewport().scale; }

    void invalidateLayout();

    void zoomIn();
    void zoomOut();
    void resetView();
    
    bool getNodeScreenRect(std::shared_ptr<Node> node, Gdk::Rectangle& rect);

    // Center the view on a specific node
    void centerViewOnNode(std::shared_ptr<Node> node);

protected:
    // Helper to zoom at a specific screen point
    void zoomAtPoint(double factor, double screenX, double screenY);

    bool on_button_press_event(GdkEventButton* event) override;
    bool on_button_release_event(GdkEventButton* event) override;
    bool on_motion_notify_event(GdkEventMotion* event) override;
    bool on_scroll_event(GdkEventScroll* event) override;
    bool on_key_press_event(GdkEventKey* event) override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
    bool on_configure_event(GdkEventConfigure* event) override;

public:
    // Signals for keyboard shortcuts
    sigc::signal<void> signal_add_child_node;
    sigc::signal<void> signal_add_sibling_node;

private:
    // Event handling helpers
    bool handleNodeSelection(GdkEventButton* event, std::shared_ptr<Node> clickedNode);
    bool handlePanningStart(GdkEventButton* event);
    bool handlePanningMove(GdkEventMotion* event);
    bool handleNodeDragMove(GdkEventMotion* event);
    
    // Navigation helper
    void navigateSelection(int directionX, int directionY);

    // Helper method to move an entire subtree by an offset
    void moveSubtree(std::shared_ptr<Node> node, double dx, double dy);
};

#endif // MAPAREA_HPP
