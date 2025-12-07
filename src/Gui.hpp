#ifndef GUI_HPP
#define GUI_HPP

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

// Forward declarations to reduce dependencies
class Node;
class MindMap;

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
        drawingContext.setRedrawCallback([this](){ this->queue_draw(); });
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
    
    void invalidateLayout() {
        drawingContext.invalidateLayout();
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

class MainWindow : public Gtk::Window {
    Gtk::Box m_VBox;
    Gtk::HeaderBar m_HeaderBar;

    std::shared_ptr<MindMap> m_Map;
    MapArea m_Area;
    std::string m_currentFilename;
    bool m_modified = false;  // Track if the document has been modified

    // Command Manager for undo/redo functionality
    CommandManager m_commandManager;

    // Clipboard for cut/copy/paste functionality
    std::shared_ptr<Node> m_clipboard;

    // (1) Acceleratori
    Glib::RefPtr<Gtk::AccelGroup> m_refAccelGroup;

    // (2) Recenti - now managed by ConfigManager
    ConfigManager m_configManager;
    Gtk::Menu* m_recentMenu = nullptr;

public:
    MainWindow();

private:
    // --- LOGICA RECENTI ---
    std::string getConfigFile();
    void loadRecentFiles();
    void saveRecentFiles();
    void addToRecent(const std::string& path);
    void rebuildRecentMenu();

    void initHeaderBar();

    void save_internal(const std::string& path);
    void open_file_internal(const std::string& path);
    void on_save();
    void on_save_as();
    void on_open();
    void on_export(std::string format);
    void on_add_node();
    void on_new();
    void on_remove_node();
    void on_map_modified();
    void open_edit_dialog(std::shared_ptr<Node> node);
    void on_undo();
    void on_redo();
    void on_about();
    void on_zoom_in();
    void on_zoom_out();
    void on_reset_view();
    void on_copy();
    void on_cut();
    void on_paste();

    // Method to check if document has been modified and prompt user to save
    bool on_delete_event(GdkEventAny* event) override;
    bool on_key_press_event(GdkEventKey* event) override;

    // Method to set modified status and update window title
    void setModified(bool modified);

    // Method to confirm save before exit
    bool confirmSaveChangesBeforeExit();
    bool on_save_as_dialog();
    void handleExport(const std::string& format, const std::string& default_filename,
                      std::function<void(Exporter&, std::shared_ptr<MindMap>, const std::string&, double)> render_func,
                      double dpi);

};

#endif // GUI_HPP