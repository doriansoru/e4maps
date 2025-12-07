#include "Gui.hpp"
#include <algorithm>  // for std::transform

MainWindow::MainWindow() : m_VBox(Gtk::ORIENTATION_VERTICAL),
                   m_Map(std::make_shared<MindMap>(_("MAIN IDEA"))),
                   m_Area(m_Map)
{
    set_title(_("E4maps - New Map"));
    set_default_size(1024, 768);
    
    // Setup HeaderBar
    m_refAccelGroup = Gtk::AccelGroup::create(); // Create the AccelGroup
    add_accel_group(m_refAccelGroup);           // Add it to the window

    // Setup HeaderBar
    initHeaderBar(); // Call initHeaderBar once, now that accelGroup is ready
    set_titlebar(m_HeaderBar);

    add(m_VBox);
    add_events(Gdk::KEY_PRESS_MASK); // Enable key press events for the window

    m_Area.signal_edit_node.connect(sigc::mem_fun(*this, &MainWindow::open_edit_dialog));
    m_Area.signal_map_modified.connect(sigc::mem_fun(*this, &MainWindow::on_map_modified));
    m_Area.set_hexpand(true); m_Area.set_vexpand(true);
    m_VBox.pack_start(m_Area);

    setModified(false);  // Initialize as not modified
    show_all();
}

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

void MainWindow::rebuildRecentMenu() {
    if (!m_recentMenu) return;
    // Clear
    auto children = m_recentMenu->get_children();
    for(auto* child : children) m_recentMenu->remove(*child);
    // Populate - using ConfigManager
    for(const auto& path : m_configManager.getRecentFiles()) {
        auto item = Gtk::manage(new Gtk::MenuItem(Glib::path_get_basename(path)));
        item->signal_activate().connect([this, path]() {
            if (confirmSaveChangesBeforeExit()) {
                open_file_internal(path);
            }
        });
        m_recentMenu->append(*item);
    }
    m_recentMenu->show_all();
}

void MainWindow::initHeaderBar() {
    m_HeaderBar.set_show_close_button(true);
    m_HeaderBar.set_title("E4maps");
    m_HeaderBar.set_subtitle(_("New Map"));

    // --- LEFT CONTROLS (File Operations) ---
    Gtk::Box* boxLeft = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    boxLeft->get_style_context()->add_class("linked");

    auto btnNew = Gtk::manage(new Gtk::Button());
    btnNew->set_image_from_icon_name("document-new", Gtk::ICON_SIZE_BUTTON);
    btnNew->set_tooltip_text(_("New Map"));
    btnNew->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_new));
    boxLeft->pack_start(*btnNew, Gtk::PACK_SHRINK);

    auto btnOpen = Gtk::manage(new Gtk::Button());
    btnOpen->set_image_from_icon_name("document-open", Gtk::ICON_SIZE_BUTTON);
    btnOpen->set_tooltip_text(_("Open Map"));
    btnOpen->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_open));
    boxLeft->pack_start(*btnOpen, Gtk::PACK_SHRINK);

    auto btnSave = Gtk::manage(new Gtk::Button());
    btnSave->set_image_from_icon_name("document-save", Gtk::ICON_SIZE_BUTTON);
    btnSave->set_tooltip_text(_("Save Map"));
    btnSave->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_save));
    boxLeft->pack_start(*btnSave, Gtk::PACK_SHRINK);

    m_HeaderBar.pack_start(*boxLeft);

    // --- RIGHT CONTROLS (Editing & Menu) ---
    
    // Hamburger Menu (Rightmost)
    auto btnMenu = Gtk::manage(new Gtk::MenuButton());
    btnMenu->set_image_from_icon_name("open-menu-symbolic", Gtk::ICON_SIZE_BUTTON);
    m_HeaderBar.pack_end(*btnMenu);

    // Add/Remove Node (Direct Access)
    auto btnRemove = Gtk::manage(new Gtk::Button());
    btnRemove->set_image_from_icon_name("list-remove", Gtk::ICON_SIZE_BUTTON);
    btnRemove->set_tooltip_text(_("Remove Branch"));
    btnRemove->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_remove_node));
    m_HeaderBar.pack_end(*btnRemove);

    auto btnAdd = Gtk::manage(new Gtk::Button());
    btnAdd->set_image_from_icon_name("list-add", Gtk::ICON_SIZE_BUTTON);
    btnAdd->set_tooltip_text(_("Add Branch"));
    btnAdd->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_node));
    m_HeaderBar.pack_end(*btnAdd);

    // Create the menu model
    auto menu = Gtk::manage(new Gtk::Menu());
    btnMenu->set_popup(*menu);

    // --- Recents ---
    auto itemRecent = Gtk::manage(new Gtk::MenuItem(_("Open Recent")));
    m_recentMenu = Gtk::manage(new Gtk::Menu());
    itemRecent->set_submenu(*m_recentMenu);
    menu->append(*itemRecent);
    rebuildRecentMenu();
    
    menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    // --- Edit Section ---
    auto itemUndo = Gtk::manage(new Gtk::MenuItem(_("Undo")));
    itemUndo->add_accelerator("activate", m_refAccelGroup, GDK_KEY_z, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemUndo->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_undo));
    menu->append(*itemUndo);
    
    auto itemRedo = Gtk::manage(new Gtk::MenuItem(_("Redo")));
    itemRedo->add_accelerator("activate", m_refAccelGroup, GDK_KEY_z, (Gdk::ModifierType)(Gdk::CONTROL_MASK | Gdk::SHIFT_MASK), Gtk::ACCEL_VISIBLE);
    itemRedo->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_redo));
    menu->append(*itemRedo);
    
    menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    auto itemCut = Gtk::manage(new Gtk::MenuItem(_("Cut")));
    itemCut->add_accelerator("activate", m_refAccelGroup, GDK_KEY_x, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemCut->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_cut));
    menu->append(*itemCut);
    
    auto itemCopy = Gtk::manage(new Gtk::MenuItem(_("Copy")));
    itemCopy->add_accelerator("activate", m_refAccelGroup, GDK_KEY_c, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemCopy->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_copy));
    menu->append(*itemCopy);
    
    auto itemPaste = Gtk::manage(new Gtk::MenuItem(_("Paste")));
    itemPaste->add_accelerator("activate", m_refAccelGroup, GDK_KEY_v, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemPaste->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_paste));
    menu->append(*itemPaste);

    menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));

    // --- View Section ---
    auto itemView = Gtk::manage(new Gtk::MenuItem(_("View")));
    auto viewSubMenu = Gtk::manage(new Gtk::Menu());
    itemView->set_submenu(*viewSubMenu);
    
    auto itemZoomIn = Gtk::manage(new Gtk::MenuItem(_("Zoom In")));
    itemZoomIn->add_accelerator("activate", m_refAccelGroup, GDK_KEY_plus, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemZoomIn->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_zoom_in));
    viewSubMenu->append(*itemZoomIn);
    
    auto itemZoomOut = Gtk::manage(new Gtk::MenuItem(_("Zoom Out")));
    itemZoomOut->add_accelerator("activate", m_refAccelGroup, GDK_KEY_minus, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemZoomOut->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_zoom_out));
    viewSubMenu->append(*itemZoomOut);
    
    auto itemReset = Gtk::manage(new Gtk::MenuItem(_("Reset View")));
    itemReset->add_accelerator("activate", m_refAccelGroup, GDK_KEY_0, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemReset->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_reset_view));
    viewSubMenu->append(*itemReset);
    
    menu->append(*itemView);

    // --- Export Section ---
    auto itemExport = Gtk::manage(new Gtk::MenuItem(_("Export")));
    auto exportSubMenu = Gtk::manage(new Gtk::Menu());
    itemExport->set_submenu(*exportSubMenu);
    
    auto itemExpPng = Gtk::manage(new Gtk::MenuItem(_("To PNG...")));
    itemExpPng->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_export), "png"));
    exportSubMenu->append(*itemExpPng);
    
    auto itemExpPdf = Gtk::manage(new Gtk::MenuItem(_("To PDF...")));
    itemExpPdf->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_export), "pdf"));
    exportSubMenu->append(*itemExpPdf);
    
    auto itemExpFp = Gtk::manage(new Gtk::MenuItem(_("To Freeplane...")));
    itemExpFp->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_export), "freeplane"));
    exportSubMenu->append(*itemExpFp);
    
    menu->append(*itemExport);

    auto itemSaveAs = Gtk::manage(new Gtk::MenuItem(_("Save As...")));
    itemSaveAs->add_accelerator("activate", m_refAccelGroup, GDK_KEY_s, (Gdk::ModifierType)(Gdk::CONTROL_MASK | Gdk::SHIFT_MASK), Gtk::ACCEL_VISIBLE);
    itemSaveAs->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_save_as));
    menu->append(*itemSaveAs);

    menu->append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    auto itemAbout = Gtk::manage(new Gtk::MenuItem(_("About")));
    itemAbout->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about));
    menu->append(*itemAbout);
    
    auto itemQuit = Gtk::manage(new Gtk::MenuItem(_("Quit")));
    itemQuit->add_accelerator("activate", m_refAccelGroup, GDK_KEY_q, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    itemQuit->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::close));
    menu->append(*itemQuit);

    menu->show_all();
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
        Gtk::MessageDialog(*this, _("Map saved successfully."), false, Gtk::MESSAGE_INFO).run();
        addToRecent(path); // Add to recent files (now handled by ConfigManager)
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
        addToRecent(path); // Update recent files order (now handled by ConfigManager)
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
    if (dialog.run() == Gtk::RESPONSE_OK) save_internal(dialog.get_filename());
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
    if (dialog.run() == Gtk::RESPONSE_OK) {
         open_file_internal(dialog.get_filename());
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

    Gtk::FileChooserDialog fileDialog(*this, dialog_title, action);
    fileDialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    fileDialog.add_button(_("Export"), Gtk::RESPONSE_OK);
    fileDialog.set_do_overwrite_confirmation(true);
    fileDialog.set_current_name(default_filename);

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
            render_func(r, m_Map, fileDialog.get_filename(), dpi);
            Gtk::MessageDialog(*this, success_message, false, Gtk::MESSAGE_INFO).run();
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
    auto selected = m_Area.getSelectedNode();
    if (!selected || selected->isRoot()) return;

    if (auto p = selected->parent.lock()) {
        auto removeCmd = std::make_unique<RemoveNodeCommand>(p, selected);
        m_commandManager.executeCommand(std::move(removeCmd));

        m_Area.invalidateLayout();
        setModified(true);
    }
}

#include "NodeEditDialog.hpp"

// ... (existing code)

void MainWindow::on_map_modified() {
    setModified(true);
}

void MainWindow::open_edit_dialog(std::shared_ptr<Node> node) {
    // Store original values for potential undo
    std::string origText = node->text;
    std::string origFont = node->fontDesc;
    Color origColor = node->color;
    Color origTextColor = node->textColor;
    std::string origImagePath = node->imagePath;
    int origImgWidth = node->imgWidth;
    int origImgHeight = node->imgHeight;
    std::string origConnText = node->connText;
    std::string origConnImagePath = node->connImagePath;

    NodeEditDialog dialog(*this, node);

    if (dialog.run() == Gtk::RESPONSE_OK) {
        // Get new values
        std::string newText = dialog.getNewText();
        std::string newFont = dialog.getNewFont();
        Color newTxtColor = dialog.getNewTextColor();
        Color newCol = dialog.getNewColor();
        
        std::string newNodeImagePath = dialog.getNewImagePath();
        int newImgWidth = dialog.getNewImgWidth();
        int newImgHeight = dialog.getNewImgHeight();

        std::string newConnText = dialog.getNewConnText();
        std::string newConnImagePath = dialog.getNewConnImagePath();

        // Create and execute the edit command
        auto editCmd = std::make_unique<EditNodeCommand>(
            node, origText, newText,
            origFont, newFont,
            origColor, newCol,
            origTextColor, newTxtColor,
            origImagePath, newNodeImagePath,
            origImgWidth, newImgWidth,
            origImgHeight, newImgHeight,
            origConnText, newConnText,
            origConnImagePath, newConnImagePath
        );

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
    auto selected = m_Area.getSelectedNode();
    if (!selected) return;

    // Create a copy command and execute it to populate the clipboard
    auto copyCmd = std::make_unique<CopyNodeCommand>(selected);
    copyCmd->execute();  // Execute immediately to get the copy

    // Store the copied node in the clipboard
    m_clipboard = copyCmd->getNodeCopy();
}

void MainWindow::on_cut() {
    auto selected = m_Area.getSelectedNode();
    if (!selected || selected->isRoot()) return;  // Can't cut the root node

    if (auto parent = selected->parent.lock()) {
        // Create a copy of the node before cutting
        auto nodeCopy = copyNodeTree(selected);

        auto cutCmd = std::make_unique<CutNodeCommand>(parent, selected);
        m_commandManager.executeCommand(std::move(cutCmd));

        // Store the copy in clipboard
        m_clipboard = nodeCopy;

        setModified(true);
        m_Area.invalidateLayout();
    }
}

void MainWindow::on_paste() {
    if (!m_clipboard) return;  // Nothing to paste

    auto selected = m_Area.getSelectedNode();
    if (!selected) return;  // Need a target to paste to

    auto pasteCmd = std::make_unique<PasteNodeCommand>(selected, m_clipboard);
    m_commandManager.executeCommand(std::move(pasteCmd));

    setModified(true);
    m_Area.invalidateLayout();
}

void MainWindow::on_about() {
    Gtk::AboutDialog dialog;
    dialog.set_program_name(_("E4Maps"));
    dialog.set_version(_("1.0.0"));
    dialog.set_copyright(_("Copyright (c) 2025 Dorian Soru <doriansoru@gmail.com>"));
    dialog.set_comments(_("A simple mind mapping application"));
    dialog.set_license_type(Gtk::LICENSE_GPL_3_0);
    dialog.set_website("https://github.com/doriansoru/e4maps");
    dialog.set_website_label(_("GitHub Repository"));

    // Add yourself as the author
    std::vector<Glib::ustring> authors = {"Dorian Soru <doriansoru@gmail.com>"};
    dialog.set_authors(authors);

    dialog.run();
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
    if (dialog.run() == Gtk::RESPONSE_OK) {
        save_internal(dialog.get_filename());
        return true;  // Allow exit if save was successful
    }
    return false;  // Cancel exit if save was cancelled
}
