#include "MainWindow.hpp"
#include "ThemeEditor.hpp"
#include "Exporter.hpp"
#include "NodeEditDialog.hpp"
#include "Utils.hpp"  // Include our utility functions
#include <algorithm>
#include <cstdlib> // For std::getenv

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

void MainWindow::save_internal(const std::string& path) {
    try {
        m_Map->saveToFile(path);
        m_currentFilename = path;
        set_title(_("E4maps - ") + Glib::path_get_basename(path));
        setModified(false);  // Mark as not modified after successful save
        addToRecent(std::filesystem::absolute(path).string()); // Add to recent files with absolute path (now handled by ConfigManager)
        updateLastUsedDirectory(path); // Update last directory after successful save
        updateStatusBar(_("Map saved successfully.")); // Show success message in status bar
    }
    catch (const std::exception& e) {
        std::string error_msg = std::string(_("Error saving file: ")) + e.what() +
                               "\n\nFile: " + path;
        Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
        // Keep modified status true if there was an error
    } catch (...) {
        std::string error_msg = std::string(_("Unknown error occurred while saving file: ")) + path;
        Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
        // Keep modified status true if there was an error
    }
}

void MainWindow::open_file_internal(const std::string& path) {
    try {
        auto newMap = MindMap::loadFromFile(path);
        if (!newMap || !newMap->root) {
            throw std::runtime_error("File contains no valid mind map data");
        }
        m_Map = newMap;
        m_Area.setMap(m_Map);
        m_currentFilename = path;
        m_commandManager.clear();  // Clear command history when loading new file
        set_title("E4maps - " + Glib::path_get_basename(path));
        setModified(false);  // Mark as not modified after loading
        addToRecent(std::filesystem::absolute(path).string()); // Update recent files order with absolute path (now handled by ConfigManager)
    } catch(const std::exception& e) {
        std::string error_msg = std::string(_("Error loading file: ")) + e.what() +
                               "\n\nFile: " + path;
        Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
    } catch(...) {
        std::string error_msg = std::string(_("Unknown error occurred while loading file: ")) + path;
        Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
    }
}

void MainWindow::on_save() { if (!m_currentFilename.empty()) save_internal(m_currentFilename); else on_save_as(); }

void MainWindow::on_save_as() {
    Gtk::FileChooserDialog dialog(*this, _("Save Map"), Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL); dialog.add_button(_("Save"), Gtk::RESPONSE_OK);
    dialog.set_do_overwrite_confirmation(true); dialog.set_current_name(_("map.e4m"));

    // Set the current directory to the last used directory or the directory of the current file
    std::string directory = getLastUsedDirectoryForDialog();
    if (!directory.empty()) {
        dialog.set_current_folder(directory);
    }

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        updateLastUsedDirectory(filename); // Update last directory after successful selection
        save_internal(filename);
    }
}

void MainWindow::on_open() {
    // If there are unsaved changes, prompt to save them first
    if (!confirmSaveChangesBeforeExit()) {
        return; // User cancelled the operation
    }

    Gtk::FileChooserDialog dialog(*this, _("Open Map"), Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL); dialog.add_button(_("Open"), Gtk::RESPONSE_OK);
    auto filter = Gtk::FileFilter::create(); filter->set_name(_("e4maps files")); filter->add_pattern("*.e4m");
    dialog.add_filter(filter);

    // Set the current directory to the last used directory or the directory of the current file
    std::string directory = getLastUsedDirectoryForDialog();
    if (!directory.empty()) {
        dialog.set_current_folder(directory);
    }

    if (dialog.run() == Gtk::RESPONSE_OK) {
         std::string filename = dialog.get_filename();
         updateLastUsedDirectory(filename); // Update last directory after successful selection
         open_file_internal(filename);
    }
}

void MainWindow::on_export(std::string format) {
    if (format == "png") {
        // For PNG export, show a dialog with DPI selection
        Gtk::Dialog exportDialog(_("Export to PNG"), *this, true);
        exportDialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
        exportDialog.add_button(_("Export"), Gtk::RESPONSE_OK);
        exportDialog.set_default_response(Gtk::RESPONSE_OK);

        Gtk::VBox vbox;
        vbox.set_spacing(10);
        vbox.set_margin_left(10);
        vbox.set_margin_right(10);
        vbox.set_margin_top(10);
        vbox.set_margin_bottom(10);

        Gtk::Label label(_("Select resolution for PNG export:"));
        vbox.pack_start(label, Gtk::PACK_SHRINK);

        Gtk::RadioButtonGroup resolutionGroup;
        Gtk::RadioButton radio72(resolutionGroup, _("72 DPI (Screen)"));
        Gtk::RadioButton radio300(resolutionGroup, _("300 DPI (High Quality)"));
        Gtk::RadioButton radio600(resolutionGroup, _("600 DPI (Maximum Quality)"));
        radio72.set_active(true); // Default to 72 DPI

        vbox.pack_start(radio72, Gtk::PACK_SHRINK);
        vbox.pack_start(radio300, Gtk::PACK_SHRINK);
        vbox.pack_start(radio600, Gtk::PACK_SHRINK);

        exportDialog.get_content_area()->pack_start(vbox);
        exportDialog.show_all_children();

        if (exportDialog.run() != Gtk::RESPONSE_OK) {
            return; // User cancelled
        }

        // Determine selected DPI
        double selectedDpi = 72.0; // Default
        if (radio300.get_active()) {
            selectedDpi = 300.0;
        } else if (radio600.get_active()) {
            selectedDpi = 600.0;
        }

        handleExport("png", "mappa.png",
                     [](Exporter& r, std::shared_ptr<MindMap> map, const std::string& filename, double dpi){
                         r.exportToPng(map, filename, dpi);
                     }, selectedDpi);
    } else if (format == "freeplane") {
        handleExport("freeplane", "mappa.mm",
                     [](Exporter& r, std::shared_ptr<MindMap> map, const std::string& filename, double dpi){
                         r.exportToFreeplane(map, filename);
                     }, 0.0); // DPI not applicable for Freeplane
    } else if (format == "pdf") {
        handleExport("pdf", "mappa.pdf",
                     [](Exporter& r, std::shared_ptr<MindMap> map, const std::string& filename, double dpi){
                         r.exportToPdf(map, filename);
                     }, 0.0); // DPI not applicable for PDF
    }
}

void MainWindow::handleExport(const std::string& format, const std::string& default_filename,
                              std::function<void(Exporter&, std::shared_ptr<MindMap>, const std::string&, double)> render_func,
                              double dpi) {
    Gtk::FileChooserAction action = Gtk::FILE_CHOOSER_ACTION_SAVE;
    std::string dialog_title;
    std::string success_message;
    std::string error_message_prefix;

    if (format == "png") {
        dialog_title = _("Export to PNG");
        success_message = _("PNG export completed successfully!");
        error_message_prefix = _("Error during PNG export: ");
    } else if (format == "pdf") {
        dialog_title = _("Export to PDF");
        success_message = _("PDF export completed successfully!");
        error_message_prefix = _("Error during PDF export: ");
    } else if (format == "freeplane") {
        dialog_title = _("Export to Freeplane");
        success_message = _("Freeplane export completed successfully!");
        error_message_prefix = _("Error during Freeplane export: ");
    } else {
        // Unknown format, should not happen
        std::cerr << "Unknown export format: " << format << std::endl;
        return;
    }

    // Generate the default filename based on the map's root text (name) if available
    std::string export_filename = default_filename;
    if (m_Map && m_Map->root && !m_Map->root->text.empty()) {
        std::string sanitized_map_name = m_Map->root->text;

        // Sanitize the filename by replacing non-alphanumeric characters with underscores
        for (auto& c : sanitized_map_name) {
            if (!std::isalnum(c) && c != '-' && c != '_') {
                c = '_';
            }
        }

        // If the sanitized name is empty, fallback to default
        if (!sanitized_map_name.empty()) {
            if (format == "png") {
                export_filename = sanitized_map_name + ".png";
            } else if (format == "pdf") {
                export_filename = sanitized_map_name + ".pdf";
            } else if (format == "freeplane") {
                export_filename = sanitized_map_name + ".mm";
            }
        }
    }

    Gtk::FileChooserDialog fileDialog(*this, dialog_title, action);
    fileDialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    fileDialog.add_button(_("Export"), Gtk::RESPONSE_OK);
    fileDialog.set_do_overwrite_confirmation(true);
    fileDialog.set_current_name(export_filename);

    // Set the current directory to the last used directory or the directory of the current file
    std::string directory = getLastUsedDirectoryForDialog();
    if (!directory.empty()) {
        fileDialog.set_current_folder(directory);
    }

    if (format == "freeplane") {
        auto filter = Gtk::FileFilter::create();
        filter->set_name(_("Freeplane files"));
        filter->add_pattern("*.mm");
        fileDialog.add_filter(filter);
    } else if (format == "png") {
        auto filter = Gtk::FileFilter::create();
        filter->set_name(_("PNG images"));
        filter->add_pattern("*.png");
        fileDialog.add_filter(filter);
    } else if (format == "pdf") {
        auto filter = Gtk::FileFilter::create();
        filter->set_name(_("PDF documents"));
        filter->add_pattern("*.pdf");
        fileDialog.add_filter(filter);
    }


    if (fileDialog.run() == Gtk::RESPONSE_OK) {
        // Renderer will be renamed to Exporter later, but for now it's Renderer
                    Exporter r(4096, 4096);        try {
            std::string filename = fileDialog.get_filename();
            updateLastUsedDirectory(filename); // Update last directory after successful selection
            render_func(r, m_Map, filename, dpi);
            updateStatusBar(success_message); // Show success message in status bar
        } catch(const std::exception& e) {
            std::string error_msg = error_message_prefix + e.what();
            Gtk::MessageDialog(*this, error_msg, false, Gtk::MESSAGE_ERROR).run();
        } catch(...) {
            Gtk::MessageDialog(*this, error_message_prefix + _("Unknown error occurred."), false, Gtk::MESSAGE_ERROR).run();
        }
    }
}

void MainWindow::on_add_node() {
    auto selected = m_Area.getSelectedNode();
    if (!selected) return;

    auto newNode = std::make_shared<Node>(_("New"), Color::random());
    auto addCmd = std::make_unique<AddNodeCommand>(selected, newNode);
    m_commandManager.executeCommand(std::move(addCmd));

    m_Area.invalidateLayout();
    setModified(true);
    open_edit_dialog(newNode);
}

void MainWindow::on_new() {
    // If there are unsaved changes, prompt to save them first
    if (!confirmSaveChangesBeforeExit()) {
        return; // User cancelled the operation
    }

    // Create a new empty map
    m_Map = std::make_shared<MindMap>(_("MAIN IDEA"));
    m_Area.setMap(m_Map);
    m_currentFilename.clear();
    m_commandManager.clear();  // Clear command history for new document
    setModified(false);  // New document is not modified initially
    set_title(_("E4maps - New Map"));
}

void MainWindow::on_remove_node() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.empty()) return;

    // Check if any of the selected nodes is the root (can't remove root)
    bool hasRootNode = false;
    for (auto& node : selectedNodes) {
        if (node && node->isRoot()) {
            hasRootNode = true;
            break;
        }
    }

    if (hasRootNode) {
        // If any node is the root, only remove non-root nodes
        std::vector<std::shared_ptr<Node>> nonRootNodes;
        for (auto& node : selectedNodes) {
            if (node && !node->isRoot()) {
                nonRootNodes.push_back(node);
            }
        }

        if (nonRootNodes.empty()) return; // Only root was selected

        // Process each non-root node for removal
        for (auto& node : nonRootNodes) {
            if (auto p = node->parent.lock()) {
                auto removeCmd = std::make_unique<RemoveNodeCommand>(p, node);
                m_commandManager.executeCommand(std::move(removeCmd));
            }
        }

        m_Area.invalidateLayout();
        setModified(true);
    } else {
        // All selected nodes are non-root, remove them all
        // Process each node for removal
        for (auto& node : selectedNodes) {
            if (auto p = node->parent.lock()) {
                auto removeCmd = std::make_unique<RemoveNodeCommand>(p, node);
                m_commandManager.executeCommand(std::move(removeCmd));
            }
        }

        m_Area.invalidateLayout();
        setModified(true);
    }
}

void MainWindow::on_map_modified() {
    setModified(true);
}

void MainWindow::open_edit_dialog(std::shared_ptr<Node> node) {
    NodeEditDialog dialog(*this, node);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        auto editCmd = dialog.createEditCommand();
        m_commandManager.executeCommand(std::move(editCmd));

        // Note: The node values are updated by the command execution

        m_Area.invalidateLayout();
        setModified(true);
    }
}

void MainWindow::on_undo() {
    if (m_commandManager.canUndo()) {
        m_commandManager.undo();
        m_Area.invalidateLayout();
        setModified(true);
    }
}

void MainWindow::on_redo() {
    if (m_commandManager.canRedo()) {
        m_commandManager.redo();
        m_Area.invalidateLayout();
        setModified(true);
    }
}

void MainWindow::on_copy() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.empty()) return;

    // Create a copy command and execute it to populate the clipboard
    auto copyCmd = std::make_unique<CopyMultipleNodesCommand>(selectedNodes);
    copyCmd->execute();  // Execute immediately to get the copy

    // Store the copied nodes in the clipboard
    m_clipboard = copyCmd->getNodesCopy();
}

void MainWindow::on_cut() {
    auto selectedNodes = m_Area.getSelectedNodes();
    if (selectedNodes.empty()) return;

    // Check if any of the selected nodes is the root (can't cut root)
    bool hasRootNode = false;
    for (auto& node : selectedNodes) {
        if (node && node->isRoot()) {
            hasRootNode = true;
            break;
        }
    }

    if (hasRootNode) {
        // If any node is the root, only cut non-root nodes
        std::vector<std::shared_ptr<Node>> nonRootNodes;
        for (auto& node : selectedNodes) {
            if (node && !node->isRoot()) {
                nonRootNodes.push_back(node);
            }
        }

        if (nonRootNodes.empty()) return; // Only root was selected

        auto cutCmd = std::make_unique<CutMultipleNodesCommand>(nonRootNodes);
        auto* cutCmdPtr = cutCmd.get();
        m_commandManager.executeCommand(std::move(cutCmd));

        // Store the copies in clipboard
        m_clipboard = cutCmdPtr->getNodesCopy();

        setModified(true);
        m_Area.invalidateLayout();
    } else {
        // All selected nodes are non-root, cut them all
        auto cutCmd = std::make_unique<CutMultipleNodesCommand>(selectedNodes);
        auto* cutCmdPtr = cutCmd.get();
        m_commandManager.executeCommand(std::move(cutCmd));

        // Store the copies in clipboard
        m_clipboard = cutCmdPtr->getNodesCopy();

        setModified(true);
        m_Area.invalidateLayout();
    }
}

void MainWindow::on_paste() {
    if (m_clipboard.empty()) return;  // Nothing to paste

    auto selected = m_Area.getSelectedNode();
    if (!selected) return;  // Need a target to paste to

    auto pasteCmd = std::make_unique<PasteMultipleNodesCommand>(selected, m_clipboard);
    
    // Store pointer to access pasted nodes after execution
    auto* pasteCmdPtr = pasteCmd.get();
    
    m_commandManager.executeCommand(std::move(pasteCmd));

    // Update selection to the newly pasted nodes
    m_Area.setSelectedNodes(pasteCmdPtr->getPastedNodes());

    setModified(true);
    m_Area.invalidateLayout();
}

void MainWindow::on_edit_theme() {
    ThemeEditor editor(*this, m_Map->theme);
    if (editor.run() == Gtk::RESPONSE_OK) {
        m_Map->theme = editor.getResult();
        m_Area.invalidateLayout();
        setModified(true);
    }
}

void MainWindow::on_help_guide() {
    std::string filename = "user_guide_en.html";

    // Check current locale to determine if we should use Italian
    const char* lang = std::getenv("LANG");
    if (lang && std::string(lang).find("it") != std::string::npos) {
        filename = "user_guide_it.html";
    }

    std::string path_str;

#ifdef _WIN32
    // On Windows, look for docs relative to the executable
    std::vector<char> path(MAX_PATH);
    if (GetModuleFileNameA(NULL, path.data(), path.size())) {
        std::string exePath(path.data());
        std::string::size_type pos = exePath.find_last_of("\\/");
        if (pos != std::string::npos) {
            std::string exeDir = exePath.substr(0, pos);

            // List of potential relative paths to check
            std::vector<std::string> searchPaths = {
                exeDir + "\\..\\share\\doc\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\share\\doc\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\..\\share\\docs\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\share\\docs\\" + APP_NAME_STR + "\\" + filename,
                exeDir + "\\share\\docs\\" + filename,
                exeDir + "\\docs\\" + filename,
                exeDir + "\\..\\docs\\" + filename,  // For NSIS installer structure
                exeDir + "\\docs\\" + filename       // Direct docs folder
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
    // On Linux, use the configured DOCDIR
    std::filesystem::path docPath(DOCDIR);
    docPath /= filename;
    if (std::filesystem::exists(docPath)) {
        path_str = docPath.string();
    } else {
         // Fallback for development
         if (std::filesystem::exists("docs/" + filename)) {
             path_str = std::filesystem::absolute("docs/" + filename).string();
         } else if (std::filesystem::exists("../docs/" + filename)) {
             path_str = std::filesystem::absolute("../docs/" + filename).string();
         }
    }
#endif

    if (!path_str.empty()) {
        // Open the documentation directly in the external browser without any dialog
        Utils::openInBrowser(*this, "file://" + path_str);
    } else {
        Gtk::MessageDialog(*this, _("Help file not found."), false, Gtk::MESSAGE_ERROR).run();
    }
}

