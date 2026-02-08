#include "MainWindow.hpp"
#include "ThemeEditor.hpp"
#include "Exporter.hpp"
#include "NodeEditDialog.hpp"
#include "Utils.hpp"  // Include our utility functions
#include "MindMapUtils.hpp"
#include "MapSerializer.hpp" // Include for MapSerializer
#include "LayoutAlgorithm.hpp" // Include for Tree Layout
#include <algorithm>
#include <cstdlib> // For std::getenv

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

// --- LOGICA RECENTI ---
void MainWindow::loadRecentFiles() {
    // Load from ConfigManager
}

void MainWindow::saveRecentFiles() {
    // Now handled by ConfigManager
}

void MainWindow::addToRecent(const std::string& path) {
    m_configManager.addToRecent(path);
    rebuildRecentMenu();
}

void MainWindow::on_zoom_in() {
    m_Area.zoomIn();
}

void MainWindow::on_zoom_out() {
    m_Area.zoomOut();
}

void MainWindow::on_reset_view() {
    m_Area.resetView();
}

bool MainWindow::save_internal(const std::string& path) {
    if (m_controller->saveMap(path)) {
        // Success
        addToRecent(std::filesystem::absolute(path).string());
        updateLastUsedDirectory(path);
        updateStatusBar(_("Map saved successfully."));
        return true;
    } else {
        std::string error_msg = std::string(_("Error saving file: ")) + path;
        Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
        return false;
    }
}

void MainWindow::open_file_internal(const std::string& path) {
    if (m_controller->loadMap(path)) {
        // Success
        addToRecent(std::filesystem::absolute(path).string());
    } else {
        std::string error_msg = std::string(_("Error loading file: ")) + path;
        Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
    }
}

void MainWindow::on_save() { 
    if (!m_controller->getFilename().empty()) 
        save_internal(m_controller->getFilename()); 
    else 
        on_save_as(); 
}

void MainWindow::on_save_as() {
    Gtk::FileChooserDialog dialog(*this, _("Save Map"), Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL); dialog.add_button(_("Save"), Gtk::RESPONSE_OK);
    dialog.set_do_overwrite_confirmation(true); dialog.set_current_name(_("map.e4m"));

    std::string directory = getLastUsedDirectoryForDialog();
    if (!directory.empty()) {
        dialog.set_current_folder(directory);
    }

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        updateLastUsedDirectory(filename); 
        save_internal(filename);
    }
}

void MainWindow::on_open() {
    if (!confirmSaveChangesBeforeExit()) {
        return; 
    }

    Gtk::FileChooserDialog dialog(*this, _("Open Map"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL); dialog.add_button(_("Open"), Gtk::RESPONSE_OK);
    auto filter = Gtk::FileFilter::create(); filter->set_name(_("e4maps files")); filter->add_pattern("*.e4m");
    dialog.add_filter(filter);

    std::string directory = getLastUsedDirectoryForDialog();
    if (!directory.empty()) {
        dialog.set_current_folder(directory);
    }

    if (dialog.run() == Gtk::RESPONSE_OK) {
         std::string filename = dialog.get_filename();
         updateLastUsedDirectory(filename); 
         open_file_internal(filename);
    }
}

void MainWindow::on_export(std::string format) {
    auto map = m_controller->getMap();
    if (!map) return;

    if (format == "png") {
        m_exportManager->exportToPng(map);
    } else if (format == "freeplane") {
        m_exportManager->exportToFreeplane(map);
    } else if (format == "pdf") {
        m_exportManager->exportToPdf(map);
    }
}

void MainWindow::on_add_node() {
    auto selected = m_Area.getSelectedNode();
    if (!selected) {
        updateStatusBar(_("Please select a parent node to add a child."));
        return;
    }

    auto newNode = std::make_shared<Node>(_("New"), E4Color::random());
    newNode->x = selected->x + 100.0;
    newNode->y = selected->y + 100.0;

    auto addCmd = std::make_unique<AddNodeCommand>(selected, newNode);
    m_controller->executeCommand(std::move(addCmd));

    m_Area.invalidateLayout();
    open_edit_dialog(newNode);
}

void MainWindow::on_new() {
    if (!confirmSaveChangesBeforeExit()) {
        return; 
    }

    m_controller->newMap();
}

void MainWindow::on_remove_node() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.empty()) {
        updateStatusBar(_("Please select a node to remove."));
        return;
    }

    bool hasRootNode = false;
    for (auto& node : selectedNodes) {
        if (node && node->isRoot()) {
            hasRootNode = true;
            break;
        }
    }

    auto map = m_controller->getMap();

    if (hasRootNode) {
        std::vector<std::shared_ptr<Node>> nonRootNodes;
        for (auto& node : selectedNodes) {
            if (node && !node->isRoot()) {
                nonRootNodes.push_back(node);
            }
        }

        if (nonRootNodes.empty()) return; 

        auto macroCmd = std::make_unique<MacroCommand>(_("Remove Multiple Nodes"));
        for (auto& node : nonRootNodes) {
            if (auto p = node->parent.lock()) {
                auto removeCmd = std::make_unique<RemoveNodeCommand>(map, p, node);
                macroCmd->addCommand(std::move(removeCmd));
            }
        }
        
        m_controller->executeCommand(std::move(macroCmd));
        m_Area.invalidateLayout();
    } else {
        auto macroCmd = std::make_unique<MacroCommand>(_("Remove Multiple Nodes"));
        for (auto& node : selectedNodes) {
            if (auto p = node->parent.lock()) {
                auto removeCmd = std::make_unique<RemoveNodeCommand>(map, p, node);
                macroCmd->addCommand(std::move(removeCmd));
            }
        }
        
        m_controller->executeCommand(std::move(macroCmd));
        m_Area.invalidateLayout();
    }
}

void MainWindow::on_map_modified() {
    // Actually the Controller should know, but View triggers it
    m_controller->setModified(true);
}

void MainWindow::open_edit_dialog(std::shared_ptr<Node> node) {
    NodeEditDialog dialog(*this, node);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        auto editCmd = dialog.createEditCommand();
        m_controller->executeCommand(std::move(editCmd));
        m_Area.invalidateLayout();
    }
}

void MainWindow::on_undo() {
    if (m_controller->canUndo()) {
        m_controller->undo();
        // Layout invalidated via signal
    }
}

void MainWindow::on_redo() {
    if (m_controller->canRedo()) {
        m_controller->redo();
        // Layout invalidated via signal
    }
}

void MainWindow::on_copy() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.empty()) return;
    
    m_controller->copy(selectedNodes);
}

void MainWindow::on_cut() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.empty()) return;
    
    m_controller->cut(selectedNodes);
    // Layout invalidated via signal
}

void MainWindow::on_paste() {
    auto selected = m_Area.getSelectedNode();
    if (!selected) {
        updateStatusBar(_("Please select a target node to paste."));
        return;
    }
    
    m_controller->paste(selected);
    // Layout invalidated via signal
}

void MainWindow::on_edit_theme() {
    auto map = m_controller->getMap();
    Theme oldTheme = map->theme;
    ThemeEditor editor(*this, map->theme);
    if (editor.run() == Gtk::RESPONSE_OK) {
        Theme newTheme = editor.getResult();
        auto cmd = std::make_unique<ChangeThemeCommand>(map, oldTheme, newTheme);
        m_controller->executeCommand(std::move(cmd));
        
        m_Area.invalidateLayout();
    }
}

void MainWindow::on_help_guide() {
    std::string filename = "user_guide_en.html";
    const char* lang = std::getenv("LANG");
    if (lang && std::string(lang).find("it") != std::string::npos) {
        filename = "user_guide_it.html";
    }

    std::string path_str;
#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::filesystem::path exePath(path);
        std::filesystem::path docPath = exePath.parent_path().parent_path() / "Resources" / "share" / "doc" / "e4maps" / filename;
        if (std::filesystem::exists(docPath)) {
             path_str = docPath.string();
        } else {
             std::filesystem::path standardPath = exePath.parent_path().parent_path() / "Resources" / "share" / "doc" / "e4maps" / filename;
             if (std::filesystem::exists(standardPath)) {
                 path_str = standardPath.string();
             }
        }
    }
#elif defined(_WIN32)
    std::vector<char> path(MAX_PATH);
    if (GetModuleFileNameA(NULL, path.data(), path.size())) {
        std::string exePath(path.data());
        std::string::size_type pos = exePath.find_last_of("\/");
        if (pos != std::string::npos) {
            std::string exeDir = exePath.substr(0, pos);
            std::vector<std::string> searchPaths = {
                exeDir + "\\..\\share\\doc\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\share\\doc\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\..\\share\\docs\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\share\\docs\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\share\\docs\\" + filename,
                exeDir + "\\docs\\" + filename,
                exeDir + "\\..\\docs\\" + filename,
                exeDir + "\\docs\\" + filename
            };
            for (const auto& potentialPath : searchPaths) {
                if (std::filesystem::exists(potentialPath)) {
                    path_str = potentialPath;
                    break;
                }
            }
        }
    }
#else
    std::filesystem::path docPath(DOCDIR);
    docPath /= filename;
    if (std::filesystem::exists(docPath)) {
        path_str = docPath.string();
    } else {
         if (std::filesystem::exists("docs/" + filename)) {
             path_str = std::filesystem::absolute("docs/" + filename).string();
         } else if (std::filesystem::exists("../docs/" + filename)) {
             path_str = std::filesystem::absolute("../docs/" + filename).string();
         }
    }
#endif

    if (!path_str.empty()) {
        Utils::openInBrowser(*this, "file://" + path_str);
    } else {
        Gtk::MessageDialog(*this, _("Help file not found."), false, Gtk::MESSAGE_ERROR).run();
    }
}

void MainWindow::on_create_connection() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.size() < 2) {
        updateStatusBar(_("Please select at least 2 nodes to connect (use Ctrl+Click to select multiple nodes)."));
        return;
    }

    auto fromNode = selectedNodes[0];
    auto macroCmd = std::make_unique<MacroCommand>(_("Add Multiple Connections"));
    auto map = m_controller->getMap();
    
    for (size_t i = 1; i < selectedNodes.size(); i++) {
        auto toNode = selectedNodes[i];
        auto addConnCmd = std::make_unique<AddConnectionCommand>(map, fromNode, toNode);
        macroCmd->addCommand(std::move(addConnCmd));
    }

    m_controller->executeCommand(std::move(macroCmd));
    m_Area.invalidateLayout();

    std::string message = _("Created ") + std::to_string(selectedNodes.size() - 1) + _(" connection(s).");
    updateStatusBar(message);
}

void MainWindow::on_auto_layout() {
    auto map = m_controller->getMap();
    if (!map || !map->root) return;
    
    // 1. Capture old positions
    std::vector<std::shared_ptr<Node>> allNodes;
    std::vector<std::pair<double, double>> oldPositions;
    
    std::function<void(std::shared_ptr<Node>)> collect = [&](std::shared_ptr<Node> n) {
        if (!n) return;
        allNodes.push_back(n);
        oldPositions.push_back({n->x, n->y});
        for (auto& child : n->children) collect(child);
    };
    collect(map->root);

    // 2. Perform layout (this modifies the nodes in memory)
    MindMapUtils::resetManualPositionsRecursive(map->root);
    
    // We need to ensure dimensions are up to date for the tree layout
    // MapArea::invalidateLayout usually triggers a background thread, 
    // but for a synchronous Command we need to do it now or rely on current values.
    LayoutAlgorithms::calculateTreeLayout(map->root);
    
    // 3. Capture new positions
    std::vector<std::pair<double, double>> newPositions;
    for (const auto& n : allNodes) {
        newPositions.push_back({n->x, n->y});
    }

    // 4. Wrap everything in a command so it's undoable
    // Note: Since calculateTreeLayout already moved the nodes, we create the command 
    // in an "executed" state conceptually, but MoveMultipleNodesCommand::execute 
    // will just re-apply the same positions if called. 
    // The most correct way is to restore old positions, then execute the command via controller.
    
    // Restore old positions briefly to let the command manager handle the execution flow
    for (size_t i = 0; i < allNodes.size(); ++i) {
        allNodes[i]->x = oldPositions[i].first;
        allNodes[i]->y = oldPositions[i].second;
    }

    auto cmd = std::make_unique<MoveMultipleNodesCommand>(allNodes, oldPositions, newPositions);
    m_controller->executeCommand(std::move(cmd));
    
    m_Area.invalidateLayout();
    m_Area.centerViewOnNode(map->root); // Re-center on root
    updateStatusBar(_("Auto-layout (Tree) applied."));
}

void MainWindow::on_remove_connection() {
    auto selectedConn = m_Area.getSelectedConnection();
    if (selectedConn) {
        auto map = m_controller->getMap();
        auto cmd = std::make_unique<RemoveConnectionCommand>(map, selectedConn->from, selectedConn->to);
        m_controller->executeCommand(std::move(cmd));
        m_Area.clearConnectionSelection(); 
        m_Area.invalidateLayout();
    }
}

void MainWindow::on_nodes_moved(const std::vector<std::shared_ptr<Node>>& nodes, const std::vector<std::pair<double, double>>& oldPos, const std::vector<std::pair<double, double>>& newPos) {
    auto cmd = std::make_unique<MoveMultipleNodesCommand>(nodes, oldPos, newPos);
    m_controller->executeCommand(std::move(cmd));
}

void MainWindow::on_connection_context_menu(GdkEventButton* event, std::shared_ptr<Connection> connection) {
    if (!connection) return;

    auto children = m_ConnectionContextMenu.get_children();
    for (auto* child : children) {
        m_ConnectionContextMenu.remove(*child);
    }

    auto itemRemove = Gtk::manage(new Gtk::MenuItem(_("Remove Connection (Del)")));
    itemRemove->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_remove_connection));
    m_ConnectionContextMenu.append(*itemRemove);

    m_ConnectionContextMenu.show_all();
    m_ConnectionContextMenu.popup(event->button, event->time);
}

// --- SEARCH IMPLEMENTATION ---

void MainWindow::on_search_toggled() {
    bool visible = m_SearchBar.get_search_mode();
    m_SearchBar.set_search_mode(!visible);
    if (!visible) {
        m_SearchEntry.grab_focus();
    } else {
        m_Area.grab_focus();
    }
}

void MainWindow::on_search_text_changed() {
    m_searchManager->performSearch(m_SearchEntry.get_text());
}

void MainWindow::on_find_next() {
    m_searchManager->next();
}

void MainWindow::on_find_prev() {
    m_searchManager->prev();
}

// --- AUTO-SAVE IMPLEMENTATION ---

void MainWindow::startAutoSaveTimer() {
    // Auto-save every 2 minutes (120000 ms)
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &MainWindow::on_autosave_timeout), 120000);
}

bool MainWindow::on_autosave_timeout() {
    // Only auto-save if the map is modified and we have a valid filename
    if (m_controller->isModified() && !m_controller->getFilename().empty()) {
        std::string filename = m_controller->getFilename();
        
        // Construct auto-save filename: .filename.e4m.autosave (hidden file)
        std::filesystem::path path(filename);
        std::string autoSaveName = "." + path.filename().string() + ".autosave";
        std::filesystem::path autoSavePath = path.parent_path() / autoSaveName;
        
        // Save to the hidden autosave file without resetting the "Modified" flag
        // or changing the current filename in the controller
        try {
            MapSerializer::save(m_controller->getMap(), autoSavePath.string());
            // Optional: update status bar briefly or just log to console
             std::cout << "[Auto-Save] Saved to " << autoSavePath << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Auto-Save] Failed: " << e.what() << std::endl;
        }
    }
    return true; // Keep the timer running
}

void MainWindow::on_add_sibling_node() {
    auto selected = m_Area.getSelectedNode();
    if (!selected) return;

    // A sibling is a child of the same parent
    auto parent = selected->parent.lock();
    if (!parent) {
        // If it's root, maybe add a child instead? Or do nothing.
        // Convention: Enter on Root adds a child usually.
        on_add_node();
        return;
    }

    auto newNode = std::make_shared<Node>(_("New"), E4Color::random());
    newNode->x = selected->x + 50.0;
    newNode->y = selected->y + 50.0;

    auto addCmd = std::make_unique<AddNodeCommand>(parent, newNode);
    m_controller->executeCommand(std::move(addCmd));

    m_Area.invalidateLayout();
    open_edit_dialog(newNode);
}