#include "MainWindow.hpp"

MainWindow::MainWindow() : m_VBox(Gtk::ORIENTATION_VERTICAL),
                   m_Map(std::make_shared<MindMap>(_("MAIN IDEA"))),
                   m_Area(m_Map),
                   m_StatusContextId(0)
{
    set_title(_("E4maps - New Map"));
    set_default_size(1024, 768);

    // Setup HeaderBar
    m_refAccelGroup = Gtk::AccelGroup::create(); // Create the AccelGroup
    add_accel_group(m_refAccelGroup);           // Add it to the window

    // Setup HeaderBar
    initHeaderBar(); // Call initHeaderBar once, now that accelGroup is ready
    set_titlebar(m_HeaderBar);

    // Add status bar
    m_StatusBar.set_margin_top(2);
    m_StatusContextId = m_StatusBar.get_context_id("main");

    add(m_VBox);
    add_events(Gdk::KEY_PRESS_MASK); // Enable key press events for the window

    m_Area.signal_edit_node.connect(sigc::mem_fun(*this, &MainWindow::open_edit_dialog));
    m_Area.signal_map_modified.connect(sigc::mem_fun(*this, &MainWindow::on_map_modified));
    m_Area.signal_node_context_menu.connect(sigc::mem_fun(*this, &MainWindow::on_node_context_menu));
    m_Area.set_hexpand(true); m_Area.set_vexpand(true);
    
    // Setup Overlay for inline editing
    m_Overlay.add(m_Area);
    
    // Setup Inline Editor
    m_InlineEditor.set_wrap_mode(Gtk::WRAP_WORD);
    m_InlineEditor.set_accepts_tab(false);
    
    // Style the editor
    auto css = Gtk::CssProvider::create();
    try {
        css->load_from_data("textview { border: 1px solid #3465a4; border-radius: 4px; padding: 4px; } text { background-color: white; color: black; }");
        m_InlineEditor.get_style_context()->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch(...) {}

    m_EditorScroll.add(m_InlineEditor);
    m_EditorScroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_EditorScroll.set_halign(Gtk::ALIGN_START);
    m_EditorScroll.set_valign(Gtk::ALIGN_START);
    m_EditorScroll.hide(); // Hidden by default
    
    m_Overlay.add_overlay(m_EditorScroll);
    m_VBox.pack_start(m_Overlay);

    m_VBox.pack_start(m_StatusBar, Gtk::PACK_SHRINK);  // Add status bar at the bottom

    // Connect Editor Signals
    m_InlineEditor.signal_key_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_editor_key_press), false);
    // Use a lambda for focus out to cleanly finish editing
    m_InlineEditor.signal_focus_out_event().connect([this](GdkEventFocus*){
        if (m_EditorScroll.is_visible()) {
            finish_inline_edit(true);
        }
        return false;
    });

    setModified(false);  // Initialize as not modified
    show_all();
    m_EditorScroll.hide(); // Ensure hidden after show_all
}

// Method to check if document has been modified and prompt user to save
bool MainWindow::on_delete_event(GdkEventAny* event) {
    return !confirmSaveChangesBeforeExit();
}

bool MainWindow::on_key_press_event(GdkEventKey* event) {
    // Check for Tab key
    if (event->keyval == GDK_KEY_Tab) {
        on_add_node();
        return true; // Event handled
    }
    // Check for Delete key
    if (event->keyval == GDK_KEY_Delete) {
        on_remove_node();
        return true; // Event handled
    }
    
    // Check for F2 to start inline editing
    if (event->keyval == GDK_KEY_F2) {
        auto node = m_Area.getSelectedNode();
        if (node) {
            start_inline_edit(node);
        }
        return true;
    }

    // Call base class's handler for other keys
    return Gtk::Window::on_key_press_event(event);
}

void MainWindow::start_inline_edit(std::shared_ptr<Node> node) {
    if (!node) return;
    
    Gdk::Rectangle rect;
    if (m_Area.getNodeScreenRect(node, rect)) {
        m_editingNode = node;
        m_InlineEditor.get_buffer()->set_text(node->text);
        
        // Position the editor
        // We set margins to position the overlay widget
        m_EditorScroll.set_margin_left(rect.get_x());
        m_EditorScroll.set_margin_top(rect.get_y());
        
        // Set size - slightly larger than node or min size
        int width = std::max(rect.get_width() + 20, 150);
        int height = std::max(rect.get_height() + 20, 50);
        m_EditorScroll.set_size_request(width, height);
        
        m_EditorScroll.show();
        m_InlineEditor.grab_focus();
        
        // Select all text
        auto buffer = m_InlineEditor.get_buffer();
        buffer->select_range(buffer->begin(), buffer->end());
    }
}

void MainWindow::finish_inline_edit(bool save) {
    if (save && m_editingNode) {
        std::string newText = m_InlineEditor.get_buffer()->get_text();
        if (newText != m_editingNode->text) {
            // Create command
            // We pass current values as both old and new for non-text properties
            auto cmd = std::make_unique<EditNodeCommand>(
                m_editingNode,
                m_editingNode->text, newText,
                m_editingNode->fontDesc, m_editingNode->fontDesc,
                m_editingNode->color, m_editingNode->color,
                m_editingNode->textColor, m_editingNode->textColor,
                m_editingNode->imagePath, m_editingNode->imagePath,
                m_editingNode->imgWidth, m_editingNode->imgWidth,
                m_editingNode->imgHeight, m_editingNode->imgHeight,
                m_editingNode->connText, m_editingNode->connText,
                m_editingNode->connImagePath, m_editingNode->connImagePath,
                m_editingNode->overrideColor, m_editingNode->overrideColor,
                m_editingNode->overrideTextColor, m_editingNode->overrideTextColor,
                m_editingNode->overrideFont, m_editingNode->overrideFont
            );
            
            m_commandManager.executeCommand(std::move(cmd));
            m_Area.invalidateLayout();
            on_map_modified();
        }
    }
    
    m_EditorScroll.hide();
    m_editingNode = nullptr;
    m_Area.grab_focus(); // Return focus to map
}

bool MainWindow::on_editor_key_press(GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Return) {
        if (event->state & GDK_SHIFT_MASK) {
            // Shift+Enter: Insert newline (default behavior), so return false
            return false; 
        } else {
            // Enter: Finish editing
            finish_inline_edit(true);
            return true;
        }
    }
    if (event->keyval == GDK_KEY_Escape) {
        finish_inline_edit(false);
        return true;
    }
    return false; // Propagate other keys
}

void MainWindow::on_node_context_menu(GdkEventButton* event, std::shared_ptr<Node> node) {
    if (!node) return;

    // Clear existing items
    auto children = m_NodeContextMenu.get_children();
    for (auto* child : children) {
        m_NodeContextMenu.remove(*child);
    }

    // 1. Edit Text (Inline)
    auto itemEdit = Gtk::manage(new Gtk::MenuItem(_("Edit Text")));
    itemEdit->signal_activate().connect([this, node]() {
        start_inline_edit(node);
    });
    m_NodeContextMenu.append(*itemEdit);

    // 2. Properties (Dialog)
    auto itemProps = Gtk::manage(new Gtk::MenuItem(_("Properties...")));
    itemProps->signal_activate().connect([this, node]() {
        open_edit_dialog(node);
    });
    m_NodeContextMenu.append(*itemProps);
    
    // Separator
    m_NodeContextMenu.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    // 3. Add Child
    auto itemAdd = Gtk::manage(new Gtk::MenuItem(_("Add Branch")));
    itemAdd->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_add_node));
    m_NodeContextMenu.append(*itemAdd);

    // 4. Remove
    if (!node->isRoot()) {
        auto itemRemove = Gtk::manage(new Gtk::MenuItem(_("Remove Branch")));
        itemRemove->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_remove_node));
        m_NodeContextMenu.append(*itemRemove);
    }

    m_NodeContextMenu.show_all();
    m_NodeContextMenu.popup(event->button, event->time);
}

// Method to set modified status and update window title
void MainWindow::setModified(bool modified) {
    m_modified = modified;

    std::string baseTitle = _("E4maps - ");
    if (!m_currentFilename.empty()) {
        baseTitle += Glib::path_get_basename(m_currentFilename);
    } else {
        baseTitle += _("New Map");
    }

    if (m_modified) {
        baseTitle += " *";  // Add asterisk to indicate unsaved changes
    }

    set_title(baseTitle);
}

// Method to confirm save before exit
bool MainWindow::confirmSaveChangesBeforeExit() {
    if (!m_modified) return true;  // No changes to save, safe to exit

    Gtk::MessageDialog dialog(*this,
        _("The document contains unsaved changes."),
        false,
        Gtk::MESSAGE_QUESTION,
        Gtk::BUTTONS_NONE);

    dialog.set_secondary_text(_("Do you want to save the changes?"));
    dialog.add_button(_("Close without saving"), Gtk::RESPONSE_NO);
    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    dialog.add_button(_("Save"), Gtk::RESPONSE_YES);

    int result = dialog.run();

    switch(result) {
        case Gtk::RESPONSE_YES:
            // Try to save the file
            if (!m_currentFilename.empty()) {
                save_internal(m_currentFilename);
                // If still modified after save (meaning error occurred), don't exit
                if (m_modified) return false;  // Don't exit if save failed
                return true;  // Exit if save was successful
            } else {
                // No filename, need to use save as dialog
                return on_save_as_dialog();
            }
        case Gtk::RESPONSE_NO:
            return true;  // Allow exit without saving
        case Gtk::RESPONSE_CANCEL:
        default:
            return false;  // Cancel exit
    }
}

// Helper method for save as dialog when exiting with unsaved changes
bool MainWindow::on_save_as_dialog() {
    Gtk::FileChooserDialog dialog(*this, _("Save Map"), Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL); dialog.add_button(_("Save"), Gtk::RESPONSE_OK);
    dialog.set_do_overwrite_confirmation(true); dialog.set_current_name("mappa.e4m");

    // Set the current directory to the last used directory or the directory of the current file
    std::string directory = getLastUsedDirectoryForDialog();
    if (!directory.empty()) {
        dialog.set_current_folder(directory);
    }

    if (dialog.run() == Gtk::RESPONSE_OK) {
        std::string filename = dialog.get_filename();
        updateLastUsedDirectory(filename); // Update last directory after successful selection
        save_internal(filename);
        return true;  // Allow exit if save was successful
    }
    return false;  // Cancel exit if save was cancelled
}

void MainWindow::updateLastUsedDirectory(const std::string& path) {
    std::filesystem::path filePath(path);
    std::string directory = filePath.parent_path().string();
    if (!directory.empty()) {
        m_configManager.saveLastUsedDirectory(directory);
    }
}

std::string MainWindow::getLastUsedDirectoryForDialog() {
    std::string lastDir = m_configManager.getLastUsedDirectory();

    // If we have a last directory and it exists, use it
    if (!lastDir.empty() && std::filesystem::exists(lastDir)) {
        return lastDir;
    }

    // If no last directory or it doesn't exist, use the directory of the current file
    if (!m_currentFilename.empty()) {
        std::filesystem::path currentPath(m_currentFilename);
        std::string currentDir = currentPath.parent_path().string();
        if (!currentDir.empty() && std::filesystem::exists(currentDir)) {
            return currentDir;
        }
    }

    // If no current file, return empty string to let GTK default behavior
    return "";
}