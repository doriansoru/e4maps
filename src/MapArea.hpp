#ifndef MAPAREA_HPP
#define MAPAREA_HPP

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/general.h>
#include <gdk/gdkkeysyms.h> // Per i tasti
#include <pangomm.h>
#include <filesystem>
#include <fstream>
#include <deque>
#include <stack>
#include "MindMap.hpp"
#include "Exporter.hpp"
#include "MindMapDrawer.hpp"  // Include for ImageCache
#include "DrawingContext.hpp"  // Include for DrawingContext and Viewport
#include "Command.hpp"  // Include for command pattern
#include "Translation.hpp"
#include "Utils.hpp"  // Include for utility functions
#include "ConfigManager.hpp"  // Include for configuration management
#include "LayoutAlgorithm.hpp"  // Include for improved layout algorithms

class MapArea : public Gtk::DrawingArea {
    DrawingContext drawingContext;

    bool isDragging = false;
    bool isPanning = false;
    double dragStartX, dragStartY;
    double panStartOffsetX, panStartOffsetY;
    double nodeStartX, nodeStartY;
    // Track previous mouse position for smooth incremental movement
    double prevMouseWorldX, prevMouseWorldY;
    bool isFirstDragMotion = true;

public:
    sigc::signal<void, std::shared_ptr<Node>> signal_edit_node;
    sigc::signal<void, GdkEventButton*, std::shared_ptr<Node>> signal_node_context_menu;
    sigc::signal<void> signal_map_modified;

    MapArea(std::shared_ptr<MindMap> m) : drawingContext(m) {
        add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                   Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
        drawingContext.setRedrawCallback([this](){ this->queue_draw(); });
    }

    std::shared_ptr<Node> getSelectedNode() const { return drawingContext.getSelectedNode(); }

    const std::vector<std::shared_ptr<Node>>& getSelectedNodes() const { return drawingContext.getSelectedNodes(); }

    void setMap(std::shared_ptr<MindMap> m) {
        drawingContext.setMap(m);
        ImageCache::getInstance().clear();
        // Center the view to show all content
        Gtk::Allocation allocation = get_allocation();
        drawingContext.centerView(allocation.get_width(), allocation.get_height());
        queue_draw();
    }

    void setSelectedNodes(const std::vector<std::shared_ptr<Node>>& nodes) {
        drawingContext.setSelectedNodes(nodes);
        queue_draw();
    }

    void invalidateLayout() {
        drawingContext.invalidateLayout();
        queue_draw();
    }

    void zoomIn();
    void zoomOut();
    void resetView();
    
    bool getNodeScreenRect(std::shared_ptr<Node> node, Gdk::Rectangle& rect);

protected:
    bool on_button_press_event(GdkEventButton* event) override;
    bool on_button_release_event(GdkEventButton* event) override;
    bool on_motion_notify_event(GdkEventMotion* event) override;
    bool on_scroll_event(GdkEventScroll* event) override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
    bool on_configure_event(GdkEventConfigure* event) override;

    // Helper method to move an entire subtree by an offset
    void moveSubtree(std::shared_ptr<Node> node, double dx, double dy);
};

#endif // MAPAREA_HPP
