#include "NodeEditDialog.hpp"
#include "Translation.hpp"
#include "Utils.hpp"
#include "Command.hpp"

NodeEditDialog::NodeEditDialog(Gtk::Window& parent, std::shared_ptr<Node> node)
    : Gtk::Dialog(_("Edit Node"), parent, true),
      m_node(node)
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
    m_origOvrC = node->overrideColor;
    m_origOvrT = node->overrideTextColor;
    m_origOvrF = node->overrideFont;

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
    m_entryText.set_text(node->text);
    grid->attach(*lblText, 0, 0, 1, 1); 
    grid->attach(m_entryText, 1, 0, 1, 1);

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
        if(!node->connImagePath.empty()) m_btnConnImg.set_filename(node->connImagePath);
        grid->attach(*lblConnImg, 0, 8, 1, 1); 
        grid->attach(m_btnConnImg, 1, 8, 1, 1);
    }

    get_content_area()->pack_start(*grid, Gtk::PACK_SHRINK);
    add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    add_button(_("Save"), Gtk::RESPONSE_OK);
    show_all_children();
}

std::string NodeEditDialog::getNewText() const {
    return m_entryText.get_text();
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

std::string NodeEditDialog::validateImage(const std::string& path, const std::string& contextName) {
    if (path.empty()) return "";
    if (!isValidImageFile(path)) {
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
    std::string newFont = getNewFont();
    Color newTxtColor = getNewTextColor();
    Color newCol = getNewColor();
    
    std::string newNodeImagePath = getNewImagePath();
    int newImgWidth = getNewImgWidth();
    int newImgHeight = getNewImgHeight();

    std::string newConnText = getNewConnText();
    std::string newConnImagePath = getNewConnImagePath();

    // Create and return the edit command
    // We assume that if the user explicitly edits/saves via the dialog, 
    // they intend to override the theme defaults for these specific properties.
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
        m_origOvrC, true, // Color override
        m_origOvrT, true, // Text Color override
        m_origOvrF, true  // Font override
    );
}
