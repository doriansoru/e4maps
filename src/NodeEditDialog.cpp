#include "NodeEditDialog.hpp"
#include "Translation.hpp"
#include "Utils.hpp"
#include "Command.hpp"

NodeEditDialog::NodeEditDialog(Gtk::Window& parent, std::shared_ptr<Node> node)
    : Gtk::Dialog(_("Edit Node"), parent, true),
      m_node(node),
      m_fontChanged(false),
      m_textColorChanged(false),
      m_connColorChanged(false),
      m_connFontChanged(false)
{
    // Capture original state
    m_origText = node->text;
    m_origFont = node->fontDesc;
    m_origColor = node->color;
    m_origTextColor = node->textColor;
    m_origImagePath = node->imagePath;
    m_origImgWidth = node->imgWidth;
    m_origImgHeight = node->imgHeight;
    m_origConnText = node->connText;
    m_origConnImagePath = node->connImagePath;
    m_origConnFontDesc = node->connFontDesc;
    m_origOvrC = node->overrideColor;
    m_origOvrT = node->overrideTextColor;
    m_origOvrF = node->overrideFont;
    m_origOvrCF = node->overrideConnFont;

    // Setup UI
    Gtk::Grid* grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10); 
    grid->set_column_spacing(10);
    grid->set_margin_left(10); 
    grid->set_margin_right(10); 
    grid->set_margin_top(10); 
    grid->set_margin_bottom(10);

    // 1. Text
    Gtk::Label* lblText = Gtk::manage(new Gtk::Label(_("Node Text:")));
    
    // Create a ScrolledWindow for the TextView
    Gtk::ScrolledWindow* textScroll = Gtk::manage(new Gtk::ScrolledWindow());
    textScroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    textScroll->set_min_content_height(100); // Set a minimum height for the text area
    
    m_entryText.set_wrap_mode(Gtk::WRAP_WORD);
    m_entryText.set_accepts_tab(false);
    m_textBuffer = Gtk::TextBuffer::create();
    m_entryText.set_buffer(m_textBuffer);
    m_textBuffer->set_text(node->text);
    
    // Style the text view to match the inline editor
    auto css = Gtk::CssProvider::create();
    try {
        css->load_from_data("scrolledwindow { border: 1px solid #3465a4; border-radius: 4px; padding: 4px; background-color: white; } entry { border: 1px solid #3465a4; border-radius: 4px; padding: 4px; } textview { border: none; background-color: transparent; }");
        textScroll->get_style_context()->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        m_entryConnText.get_style_context()->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        m_entryText.get_style_context()->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } catch(...) {}
    
    textScroll->add(m_entryText);
    
    grid->attach(*lblText, 0, 0, 1, 1); 
    grid->attach(*textScroll, 1, 0, 1, 1);
    
    // Connect key press event for Shift+Enter handling
    m_entryText.signal_key_press_event().connect(sigc::mem_fun(*this, &NodeEditDialog::on_text_key_press), false);

    // 2. Font and Text Color
    Gtk::Label* lblFont = Gtk::manage(new Gtk::Label(_("Font:")));
    m_btnFont.set_font_name(node->fontDesc);
    grid->attach(*lblFont, 0, 1, 1, 1); 
    grid->attach(m_btnFont, 1, 1, 1, 1);

    Gtk::Label* lblTextColor = Gtk::manage(new Gtk::Label(_("Text Color:")));
    Gdk::RGBA currentTextColor; 
    currentTextColor.set_rgba(node->textColor.r, node->textColor.g, node->textColor.b);
    m_btnTextColor.set_rgba(currentTextColor);
    grid->attach(*lblTextColor, 0, 2, 1, 1); 
    grid->attach(m_btnTextColor, 1, 2, 1, 1);

    // 3. Connection Color (Hidden if Root)
    Gtk::Label* lblColorConn = Gtk::manage(new Gtk::Label(_("Connection Color:")));
    
    if (!node->isRoot()) {
        Gdk::RGBA currentColorConn; 
        currentColorConn.set_rgba(node->color.r, node->color.g, node->color.b);
        m_colorBtnConn.set_rgba(currentColorConn);
        grid->attach(*lblColorConn, 0, 3, 1, 1); 
        grid->attach(m_colorBtnConn, 1, 3, 1, 1);
    }

    // 4. Image + Dimensions
    Gtk::Label* lblImg = Gtk::manage(new Gtk::Label(_("Node Image:")));
    Gtk::Box* boxImg = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    
    m_btnImg.set_title(_("Choose Image"));
    m_btnImg.set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
    if (!node->imagePath.empty()) m_btnImg.set_filename(node->imagePath);
    
    auto filter = Gtk::FileFilter::create(); 
    filter->set_name(_("Images"));
    filter->add_mime_type("image/png"); 
    filter->add_mime_type("image/jpeg"); 
    filter->add_mime_type("image/gif");
    m_btnImg.add_filter(filter);
    
    boxImg->pack_start(m_btnImg);
    
    // Add clear button for node image
    m_btnClearImg.set_label(_("Clear"));
    m_btnClearImg.signal_clicked().connect(sigc::mem_fun(*this, &NodeEditDialog::on_clear_image_clicked));
    boxImg->pack_start(m_btnClearImg, Gtk::PACK_SHRINK);

    Gtk::Label* lblW = Gtk::manage(new Gtk::Label(_("W:"))); 
    m_spinW.set_adjustment(Gtk::Adjustment::create(node->imgWidth, 0, 2000, 10));
    m_spinW.set_digits(0);
    
    Gtk::Label* lblH = Gtk::manage(new Gtk::Label(_("H:"))); 
    m_spinH.set_adjustment(Gtk::Adjustment::create(node->imgHeight, 0, 2000, 10));
    m_spinH.set_digits(0);
    
    boxImg->pack_start(*lblW); 
    boxImg->pack_start(m_spinW);
    boxImg->pack_start(*lblH); 
    boxImg->pack_start(m_spinH);

    grid->attach(*lblImg, 0, 4, 1, 1); 
    grid->attach(*boxImg, 1, 4, 1, 1);

    // --- Connection ---
    m_btnConnImg.set_title(_("Choose Icon"));
    m_btnConnImg.set_action(Gtk::FILE_CHOOSER_ACTION_OPEN);
    m_btnConnImg.add_filter(filter); // Use same filter

    if (!node->isRoot()) {
        Gtk::Separator* sep = Gtk::manage(new Gtk::Separator());
        grid->attach(*sep, 0, 5, 2, 1);
        
        Gtk::Label* lblConnTitle = Gtk::manage(new Gtk::Label(_("--- Branch Annotation ---")));
        grid->attach(*lblConnTitle, 0, 6, 2, 1);

        Gtk::Label* lblConnText = Gtk::manage(new Gtk::Label(_("Branch Text:")));
        m_entryConnText.set_text(node->connText);
        grid->attach(*lblConnText, 0, 7, 1, 1); 
        grid->attach(m_entryConnText, 1, 7, 1, 1);

        Gtk::Label* lblConnImg = Gtk::manage(new Gtk::Label(_("Branch Icon:")));
        Gtk::Box* boxConnImg = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        if(!node->connImagePath.empty()) m_btnConnImg.set_filename(node->connImagePath);
        
        boxConnImg->pack_start(m_btnConnImg);
        
        // Add clear button for connection image
        m_btnClearConnImg.set_label(_("Clear"));
        m_btnClearConnImg.signal_clicked().connect(sigc::mem_fun(*this, &NodeEditDialog::on_clear_conn_image_clicked));
        boxConnImg->pack_start(m_btnClearConnImg, Gtk::PACK_SHRINK);
        
        grid->attach(*lblConnImg, 0, 8, 1, 1); 
        grid->attach(*boxConnImg, 1, 8, 1, 1);

        Gtk::Label* lblConnFont = Gtk::manage(new Gtk::Label(_("Branch Font:")));
        if (node->overrideConnFont && !node->connFontDesc.empty()) {
            m_btnConnFont.set_font_name(node->connFontDesc);
        } else {
            // Default to what MindMapDrawer uses for connection text
            m_btnConnFont.set_font_name("Sans Italic 12"); 
        }
        grid->attach(*lblConnFont, 0, 9, 1, 1);
        grid->attach(m_btnConnFont, 1, 9, 1, 1);
    }

    // Connect change tracking signals
    m_btnFont.signal_font_set().connect([this](){ m_fontChanged = true; });
    m_btnTextColor.signal_color_set().connect([this](){ m_textColorChanged = true; });
    
    if (!node->isRoot()) {
        m_colorBtnConn.signal_color_set().connect([this](){ m_connColorChanged = true; });
        m_btnConnFont.signal_font_set().connect([this](){ m_connFontChanged = true; });
    }

    get_content_area()->pack_start(*grid, Gtk::PACK_SHRINK);
    add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    add_button(_("Save"), Gtk::RESPONSE_OK);
    show_all_children();
}

std::string NodeEditDialog::getNewText() const {
    return m_textBuffer->get_text();
}

std::string NodeEditDialog::getNewFont() const {
    return m_btnFont.get_font_name();
}

Color NodeEditDialog::getNewTextColor() const {
    auto rgba = m_btnTextColor.get_rgba();
    return {rgba.get_red(), rgba.get_green(), rgba.get_blue()};
}

Color NodeEditDialog::getNewColor() const {
    if (m_node->isRoot()) return m_node->color;
    auto rgba = m_colorBtnConn.get_rgba();
    return {rgba.get_red(), rgba.get_green(), rgba.get_blue()};
}

std::string NodeEditDialog::getNewConnText() const {
    return m_entryConnText.get_text();
}

int NodeEditDialog::getNewImgWidth() const {
    return m_spinW.get_value_as_int();
}

int NodeEditDialog::getNewImgHeight() const {
    return m_spinH.get_value_as_int();
}

std::string NodeEditDialog::getNewConnFont() const {
    // Only applies to non-root nodes
    if (m_node->isRoot()) return m_node->connFontDesc; 
    return m_btnConnFont.get_font_name();
}

std::string NodeEditDialog::validateImage(const std::string& path, const std::string& contextName) {
    if (path.empty()) return "";
    if (!Utils::isValidImageFile(path)) {
        Gtk::MessageDialog warnDialog(*this,
            _("Selected image file format is not supported. Image will not be loaded."),
            false, Gtk::MESSAGE_WARNING);
        warnDialog.run();
        return ""; // Clear invalid path
    }
    return path;
}

std::string NodeEditDialog::getNewImagePath() {
    return validateImage(m_btnImg.get_filename(), _("Node Image"));
}

std::string NodeEditDialog::getNewConnImagePath() {
    if (m_node->isRoot()) return "";
    return validateImage(m_btnConnImg.get_filename(), _("Connection Image"));
}

std::unique_ptr<EditNodeCommand> NodeEditDialog::createEditCommand() {
    // Get new values from UI
    std::string newText = getNewText();
    
    // Logic for properties with overrides:
    // Only use the new value and set override to true if the user changed it.
    // Otherwise, keep the original value and original override state.

    std::string newFont = m_origFont;
    bool newOvrF = m_origOvrF;
    if (m_fontChanged) {
        newFont = getNewFont();
        newOvrF = true;
    }

    Color newTxtColor = m_origTextColor;
    bool newOvrT = m_origOvrT;
    if (m_textColorChanged) {
        newTxtColor = getNewTextColor();
        newOvrT = true;
    }

    Color newCol = m_origColor;
    bool newOvrC = m_origOvrC;
    if (m_connColorChanged && !m_node->isRoot()) {
        newCol = getNewColor();
        newOvrC = true;
    }

    std::string newConnFont = m_origConnFontDesc;
    bool newOvrCF = m_origOvrCF;
    if (m_connFontChanged && !m_node->isRoot()) {
        newConnFont = getNewConnFont();
        newOvrCF = true;
    }

    std::string newNodeImagePath = getNewImagePath();
    int newImgWidth = getNewImgWidth();
    int newImgHeight = getNewImgHeight();

    std::string newConnText = getNewConnText();
    std::string newConnImagePath = getNewConnImagePath();

    // Create and return the edit command
    return std::make_unique<EditNodeCommand>(
        m_node, m_origText, newText,
        m_origFont, newFont,
        m_origColor, newCol,
        m_origTextColor, newTxtColor,
        m_origImagePath, newNodeImagePath,
        m_origImgWidth, newImgWidth,
        m_origImgHeight, newImgHeight,
        m_origConnText, newConnText,
        m_origConnImagePath, newConnImagePath,
        m_origConnFontDesc, newConnFont,
        m_origOvrC, newOvrC,
        m_origOvrT, newOvrT,
        m_origOvrF, newOvrF,
        m_origOvrCF, newOvrCF
    );
}

bool NodeEditDialog::on_text_key_press(GdkEventKey* event) {
    if (event->keyval == GDK_KEY_Return) {
        if (event->state & GDK_SHIFT_MASK) {
            // Shift+Enter: Insert newline (default behavior), so return false
            return false; 
        } else {
            // Enter: Finish editing (save and close dialog)
            response(Gtk::RESPONSE_OK);
            return true;
        }
    }
    if (event->keyval == GDK_KEY_Escape) {
        response(Gtk::RESPONSE_CANCEL);
        return true;
    }
    return false; // Propagate other keys
}

void NodeEditDialog::on_clear_image_clicked() {
    // Clear the image selection
    m_btnImg.unselect_all();
    m_btnImg.set_filename("");
    // Reset dimensions to auto (0)
    m_spinW.set_value(0);
    m_spinH.set_value(0);
}

void NodeEditDialog::on_clear_conn_image_clicked() {
    // Clear the connection image selection
    m_btnConnImg.unselect_all();
    m_btnConnImg.set_filename("");
}
