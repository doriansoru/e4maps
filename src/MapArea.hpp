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

public:
    sigc::signal<void, std::shared_ptr<Node>> signal_edit_node;
    sigc::signal<void> signal_map_modified;

    MapArea(std::shared_ptr<MindMap> m) : drawingContext(m) {
        add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
                   Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
    }

    std::shared_ptr<Node> getSelectedNode() const { return drawingContext.getSelectedNode(); }

    void setMap(std::shared_ptr<MindMap> m) {
        drawingContext.setMap(m);
        ImageCache::getInstance().clear();
        // Center the view to show all content
        Gtk::Allocation allocation = get_allocation();
        drawingContext.centerView(allocation.get_width(), allocation.get_height());
        queue_draw();
    }

    void zoomIn();
    void zoomOut();
    void resetView();

protected:
    bool on_button_press_event(GdkEventButton* event) override;
    bool on_button_release_event(GdkEventButton* event) override;
    bool on_motion_notify_event(GdkEventMotion* event) override;
    bool on_scroll_event(GdkEventScroll* event) override;
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
    bool on_configure_event(GdkEventConfigure* event) override;
};

#endif // MAPAREA_HPP
