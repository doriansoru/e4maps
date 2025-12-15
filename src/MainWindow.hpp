#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <gtkmm.h>
#include <gtkmm/overlay.h>
#include <gtkmm/textview.h>
#include <gtkmm/scrolledwindow.h>
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
#include "MapArea.hpp"  // Include for MapArea class definition

// Forward declarations to reduce dependencies
class Node;
class MindMap;

class MainWindow : public Gtk::Window {
    Gtk::Box m_VBox;
    Gtk::HeaderBar m_HeaderBar;
    Gtk::Statusbar m_StatusBar;
    guint m_StatusContextId;

    // Inline editing components
    Gtk::Overlay m_Overlay;
    Gtk::ScrolledWindow m_EditorScroll;
    Gtk::TextView m_InlineEditor;
    std::shared_ptr<Node> m_editingNode;
    sigc::connection m_editorFocusOutConn;
    Glib::RefPtr<Gtk::CssProvider> m_dynamicCssProvider;
    
    // Context Menu
    Gtk::Menu m_NodeContextMenu;

    std::shared_ptr<MindMap> m_Map;
    MapArea m_Area;
    std::string m_currentFilename;
    bool m_modified = false;  // Track if the document has been modified

    // Command Manager for undo/redo functionality
    CommandManager m_commandManager;

    // Clipboard for cut/copy/paste functionality
    std::vector<std::shared_ptr<Node>> m_clipboard;

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

public:
    void openFile(const std::string& path) { open_file_internal(path); }

private:
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
    void on_edit_theme();
    void on_help_guide();

    // Inline editing methods
    void start_inline_edit(std::shared_ptr<Node> node);
    void finish_inline_edit(bool save);
    bool on_editor_key_press(GdkEventKey* event);
    void on_node_context_menu(GdkEventButton* event, std::shared_ptr<Node> node);

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

    // Helper method for status bar
    void updateStatusBar(const std::string& message);

    // Helper methods for file dialog management
    void updateLastUsedDirectory(const std::string& path);
    std::string getLastUsedDirectoryForDialog();

};

#endif // MAINWINDOW_HPP