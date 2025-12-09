#include "MainWindow.hpp"
#include "ThemeEditor.hpp"
#include "Exporter.hpp"

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
    
    // --- Theme Section ---
    auto itemEditTheme = Gtk::manage(new Gtk::MenuItem(_("Edit Theme...")));
    itemEditTheme->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_edit_theme));
    menu->append(*itemEditTheme);
    
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

void MainWindow::updateStatusBar(const std::string& message) {
    m_StatusBar.pop(m_StatusContextId);  // Clear previous message
    m_StatusBar.push(message, m_StatusContextId);  // Push new message
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
