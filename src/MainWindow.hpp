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
#include "DrawingContext.hpp"  // Include for DrawingContext and Viewport
#include "Command.hpp"  // Include for command pattern
#include "Translation.hpp"
#include "Utils.hpp"  // Include for utility functions
#include "ConfigManager.hpp"  // Include for configuration management
#include "MapArea.hpp"  // Include for MapArea class definition
#include "MapController.hpp" // Include MapController
#include "SearchManager.hpp" // Include SearchManager
#include "ExportManager.hpp" // Include ExportManager

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
    Gtk::Menu m_ConnectionContextMenu; // Context menu for connections

    // Search components
    Gtk::SearchBar m_SearchBar;
    Gtk::SearchEntry m_SearchEntry;
    Gtk::Button m_ButtonFindNext;
    Gtk::Button m_ButtonFindPrev;
    
    // Logic extracted to Managers
    std::unique_ptr<SearchManager> m_searchManager;
    std::unique_ptr<ExportManager> m_exportManager;

    // Controller
    std::unique_ptr<MapController> m_controller;
    
    MapArea m_Area;
    // Removed direct state management (moved to Controller)
    // std::shared_ptr<MindMap> m_Map;
    // std::string m_currentFilename;
    // bool m_modified = false;
    // CommandManager m_commandManager;
    // Clipboard
    // std::vector<std::shared_ptr<Node>> m_clipboard;
    // std::vector<std::shared_ptr<Connection>> m_clipboardConnections;

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
    bool save_internal(const std::string& path);
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
    void on_auto_layout();
    void on_create_connection();
    void on_remove_connection();
    void on_nodes_moved(const std::vector<std::shared_ptr<Node>>& nodes, const std::vector<std::pair<double, double>>& oldPos, const std::vector<std::pair<double, double>>& newPos);
    void on_connection_context_menu(GdkEventButton* event, std::shared_ptr<Connection> connection);
    
    // Auto-save logic
    bool on_autosave_timeout();
    void startAutoSaveTimer();
    
    // Keyboard shortcuts handlers from MapArea
    void on_add_sibling_node();
    
    // Search methods
    void on_search_toggled();
    void on_search_text_changed();
    void on_find_next();
    void on_find_prev();

    // Inline editing methods
    void setupInlineEditor();
    std::string generateCssForNode(std::shared_ptr<Node> node, double scale);
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
    // handleExport moved to ExportManager

    // Helper method for status bar
    void updateStatusBar(const std::string& message);

    // Helper methods for file dialog management
    void updateLastUsedDirectory(const std::string& path);
    std::string getLastUsedDirectoryForDialog();

};

#endif // MAINWINDOW_HPP