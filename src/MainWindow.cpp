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
    m_Area.set_hexpand(true); m_Area.set_vexpand(true);
    m_VBox.pack_start(m_Area);
    m_VBox.pack_start(m_StatusBar, Gtk::PACK_SHRINK);  // Add status bar at the bottom

    setModified(false);  // Initialize as not modified
    show_all();
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

    // Call base class's handler for other keys
    return Gtk::Window::on_key_press_event(event);
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