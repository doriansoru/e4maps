#include "MainWindow.hpp"

MainWindow::MainWindow() : m_VBox(Gtk::ORIENTATION_VERTICAL),
                   m_controller(std::make_unique<MapController>()),
                   m_Area(m_controller->getMap()),
                   m_StatusContextId(0)
{
    // Initialize Managers
    m_searchManager = std::make_unique<SearchManager>(*m_controller, m_Area);
    m_exportManager = std::make_unique<ExportManager>(*this);
    m_exportManager->setStatusCallback([this](const std::string& msg){ updateStatusBar(msg); });
    
    set_title(_("E4maps - New Map"));
    set_default_size(1024, 768);

    // Setup HeaderBar
    m_refAccelGroup = Gtk::AccelGroup::create(); // Create the AccelGroup
    add_accel_group(m_refAccelGroup);           // Add it to the window

    // Setup HeaderBar
    initHeaderBar(); // Call initHeaderBar once, now that accelGroup is ready
    set_titlebar(m_HeaderBar);

    // --- Search Bar Setup ---
    m_SearchBar.connect_entry(m_SearchEntry);
    
    auto searchBox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    searchBox->pack_start(m_SearchEntry, Gtk::PACK_EXPAND_WIDGET);
    
    m_ButtonFindPrev.set_image_from_icon_name("go-up-symbolic", Gtk::ICON_SIZE_BUTTON);
    m_ButtonFindPrev.set_tooltip_text(_("Previous Match"));
    m_ButtonFindNext.set_image_from_icon_name("go-down-symbolic", Gtk::ICON_SIZE_BUTTON);
    m_ButtonFindNext.set_tooltip_text(_("Next Match"));
    
    auto navBox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    navBox->get_style_context()->add_class("linked");
    navBox->pack_start(m_ButtonFindPrev, Gtk::PACK_SHRINK);
    navBox->pack_start(m_ButtonFindNext, Gtk::PACK_SHRINK);
    
    searchBox->pack_start(*navBox, Gtk::PACK_SHRINK);
    m_SearchBar.add(*searchBox);
    
    m_VBox.pack_start(m_SearchBar, Gtk::PACK_SHRINK);

    // Connect search signals
    m_SearchEntry.signal_search_changed().connect(sigc::mem_fun(*this, &MainWindow::on_search_text_changed));
    m_ButtonFindNext.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_find_next));
    m_ButtonFindPrev.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_find_prev));

    // Setup Search Manager Callback
    m_searchManager->setStatusCallback([this](const std::string& msg, bool isError) {
        updateStatusBar(msg);
        if (isError) {
            m_SearchEntry.get_style_context()->add_class("error");
        } else {
            m_SearchEntry.get_style_context()->remove_class("error");
        }
    });

    // Add status bar
    m_StatusBar.set_margin_top(2);
    m_StatusContextId = m_StatusBar.get_context_id("main");

    add(m_VBox);
    add_events(Gdk::KEY_PRESS_MASK); // Enable key press events for the window

    m_Area.signal_edit_node.connect(sigc::mem_fun(*this, &MainWindow::open_edit_dialog));
    m_Area.signal_map_modified.connect(sigc::mem_fun(*this, &MainWindow::on_map_modified));
    m_Area.signal_node_context_menu.connect(sigc::mem_fun(*this, &MainWindow::on_node_context_menu));
    // Connect connection context menu signal
    m_Area.signal_connection_context_menu.connect(sigc::mem_fun(*this, &MainWindow::on_connection_context_menu));
    // Connect the move signal for Undo support
    m_Area.signal_nodes_moved.connect(sigc::mem_fun(*this, &MainWindow::on_nodes_moved));
    // Connect keyboard shortcut signals
    m_Area.signal_add_child_node.connect(sigc::mem_fun(*this, &MainWindow::on_add_node));
    m_Area.signal_add_sibling_node.connect(sigc::mem_fun(*this, &MainWindow::on_add_sibling_node));

    m_Area.set_hexpand(true); m_Area.set_vexpand(true);
    
    // Connect Controller Signals
    m_controller->signal_map_changed.connect([this]() {
        // Update View with new map
        m_Area.setMap(m_controller->getMap());
        // Update Title
        setModified(m_controller->isModified());
    });
    
    m_controller->signal_modified_changed.connect([this](bool mod) {
        setModified(mod);
    });
    
    m_controller->signal_layout_invalidated.connect([this]() {
        m_Area.invalidateLayout();
    });

    // Setup Overlay for inline editing
    m_Overlay.add(m_Area);
    
    setupInlineEditor();
    
    m_Overlay.add_overlay(m_EditorScroll);
    m_VBox.pack_start(m_Overlay);

    m_VBox.pack_start(m_StatusBar, Gtk::PACK_SHRINK);  // Add status bar at the bottom

    setModified(false);  // Initialize as not modified
    startAutoSaveTimer(); // Start the auto-save mechanism
    
    show_all();
    m_EditorScroll.hide(); // Ensure hidden after show_all
}

void MainWindow::setupInlineEditor() {
    // Setup Inline Editor
    m_InlineEditor.set_wrap_mode(Gtk::WRAP_WORD);
    m_InlineEditor.set_accepts_tab(false);
    
    // Style the editor
    // Base style for structural properties
    auto baseCss = Gtk::CssProvider::create();
    try {
        // Removed specific colors from base style to allow dynamic overrides
        baseCss->load_from_data("textview { border: 1px solid #3465a4; border-radius: 4px; padding: 4px; }");
        m_InlineEditor.get_style_context()->add_provider(baseCss, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch(...) {}

    // Initialize dynamic provider
    m_dynamicCssProvider = Gtk::CssProvider::create();
    m_InlineEditor.get_style_context()->add_provider(m_dynamicCssProvider, GTK_STYLE_PROVIDER_PRIORITY_USER);

    m_EditorScroll.add(m_InlineEditor);
    m_EditorScroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_EditorScroll.set_halign(Gtk::ALIGN_START);
    m_EditorScroll.set_valign(Gtk::ALIGN_START);
    m_EditorScroll.hide(); // Hidden by default

    // Connect Editor Signals
    m_InlineEditor.signal_key_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_editor_key_press), false);
    // Use a lambda for focus out to cleanly finish editing
    m_InlineEditor.signal_focus_out_event().connect([this](GdkEventFocus*){
        if (m_EditorScroll.is_visible()) {
            finish_inline_edit(true);
        }
        return false;
    });
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
        // If a node is selected, remove node
        if (m_Area.getSelectedNode()) {
            on_remove_node();
            return true;
        }
        // If a connection is selected, remove connection
        if (m_Area.getSelectedConnection()) {
            on_remove_connection();
            return true;
        }
    }
    
    // Check for F2 to start inline editing
    if (event->keyval == GDK_KEY_F2) {
        auto node = m_Area.getSelectedNode();
        if (node) {
            start_inline_edit(node);
        }
        return true;
    }

    // Check for Ctrl+L to create connections
    if (event->keyval == GDK_KEY_l && (event->state & GDK_CONTROL_MASK)) {
        on_create_connection();
        return true;
    }
    
    // Ctrl+F for Search
    if (event->keyval == GDK_KEY_f && (event->state & GDK_CONTROL_MASK)) {
        on_search_toggled();
        return true;
    }

    // F3 for Find Next, Shift+F3 for Find Prev
    if (event->keyval == GDK_KEY_F3) {
        if (event->state & GDK_SHIFT_MASK) {
            on_find_prev();
        } else {
            on_find_next();
        }
        return true;
    }

    // Call base class's handler for other keys
    return Gtk::Window::on_key_press_event(event);
}

std::string MainWindow::generateCssForNode(std::shared_ptr<Node> node, double scale) {
    if (!node) return "";
    
    // Determine Node Style (Theme + Overrides)
    int depth = 0;
    auto p = node->parent.lock();
    while(p) { depth++; p = p->parent.lock(); }
    
    NodeStyle style = m_controller->getMap()->theme.getStyle(depth);
    
    // Resolve Colors
    double tr = 0, tg = 0, tb = 0, ta = 1;
    double br = 1, bg = 1, bb = 1, ba = 1;

    // Text E4Color
    if (node->overrideTextColor) {
        tr = node->textColor.r; tg = node->textColor.g; tb = node->textColor.b;
    } else {
        auto solid = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(style.textColor);
        if (solid) solid->get_rgba(tr, tg, tb, ta);
    }

    // Background E4Color
    auto solidBg = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(style.backgroundColor);
    if (solidBg) solidBg->get_rgba(br, bg, bb, ba);
    
    // Convert to Hex
    std::string textHex = Utils::cairoToHex(tr, tg, tb, ta);
    std::string bgHex = Utils::cairoToHex(br, bg, bb, ba);        

    // Resolve Font & Scale
    std::string fontStr = (node->overrideFont && !node->fontDesc.empty()) 
                          ? node->fontDesc : (std::string)style.fontDescription.to_string();
    
    Pango::FontDescription pfd(fontStr);
    // Scale the font size
    int sizePango = pfd.get_size();
    double sizePixels = (double)sizePango / Pango::SCALE;
    
    if (!pfd.get_size_is_absolute()) {
        // Convert points to pixels (Standard 96 DPI / 72 Points per inch = 1.3333...)
        sizePixels *= (96.0 / 72.0);
    }
    
    sizePixels *= scale;
    // Ensure minimum visible font size
    if (sizePixels < 8.0) sizePixels = 8.0;

    // Calculate Padding (Scaled)
    // Default to 4px if style padding is small, but try to match
    int pad = (int)(style.horizontalPadding * scale);
    if (pad < 2) pad = 2;

    // --- Generate CSS ---
    std::ostringstream cssData;
    cssData << "textview { "
            << "background-color: " << bgHex << "; "
            << "background-image: none; "
            << "color: " << textHex << "; "
            << "caret-color: " << textHex << "; "
            << "font-family: '" << pfd.get_family() << "'; "
            << "font-size: " << (int)sizePixels << "px; " 
            << "font-weight: " << (pfd.get_weight() >= Pango::WEIGHT_BOLD ? "bold" : "normal") << "; "
            << "font-style: " << (pfd.get_style() == Pango::STYLE_ITALIC ? "italic" : "normal") << "; "
            << "border: 1px solid alpha(" << textHex << ", 0.3); " 
            << "border-radius: " << (int)(style.cornerRadius * scale) << "px; " 
            << "padding: " << pad << "px; " 
            << "}"
            << "textview text { "
            << "background-color: " << bgHex << "; "
            << "background-image: none; "
            << "color: " << textHex << "; "
            << "}"
            << "selection { "
            << "background-color: alpha(@theme_selected_bg_color, 0.5); "
            << "color: @theme_selected_fg_color; "
            << "}";
    
    return cssData.str();
}

void MainWindow::start_inline_edit(std::shared_ptr<Node> node) {
    if (!node) return;
    
    Gdk::Rectangle rect;
    if (m_Area.getNodeScreenRect(node, rect)) {
        m_editingNode = node;
        m_InlineEditor.get_buffer()->set_text(node->text);
        
        std::string cssData = generateCssForNode(node, m_Area.getScale());
             
        try {
            // Load data into the existing provider which is already attached with PRIORITY_USER
            m_dynamicCssProvider->load_from_data(cssData);
        } catch(const Gtk::CssProviderError& ex) {
            std::cerr << "CSS Error: " << ex.what() << std::endl;
        } catch(...) {
            std::cerr << "Unknown CSS Error" << std::endl;
        }
        
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
                m_editingNode->connFontDesc, m_editingNode->connFontDesc,
                m_editingNode->overrideColor, m_editingNode->overrideColor,
                m_editingNode->overrideTextColor, m_editingNode->overrideTextColor,
                m_editingNode->overrideFont, m_editingNode->overrideFont,
                m_editingNode->overrideConnFont, m_editingNode->overrideConnFont
            );
            
            m_controller->executeCommand(std::move(cmd));
            // m_Area layout is updated via signal connection
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
    // Note: MainWindow doesn't own m_modified anymore, but we still use this to update title
    // Controller manages state, this just updates the view (title)
    
    std::string baseTitle = _("E4maps - ");
    std::string filename = m_controller->getFilename();
    
    if (!filename.empty()) {
        baseTitle += Glib::path_get_basename(filename);
    } else {
        baseTitle += _("New Map");
    }

    if (modified) {
        baseTitle += " *";  // Add asterisk to indicate unsaved changes
    }

    set_title(baseTitle);
}

// Method to confirm save before exit
bool MainWindow::confirmSaveChangesBeforeExit() {
    if (!m_controller->isModified()) return true;  // No changes to save, safe to exit

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
            if (!m_controller->getFilename().empty()) {
                if (save_internal(m_controller->getFilename())) {
                     return true; // Exit if save successful
                } else {
                     return false; // Don't exit if save failed
                }
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
        if (save_internal(filename)) {
            return true;  // Allow exit if save was successful
        }
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
    std::string filename = m_controller->getFilename();
    if (!filename.empty()) {
        std::filesystem::path currentPath(filename);
        std::string currentDir = currentPath.parent_path().string();
        if (!currentDir.empty() && std::filesystem::exists(currentDir)) {
            return currentDir;
        }
    }

    // If no current file, return empty string to let GTK default behavior
    return "";
}