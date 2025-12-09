#ifndef NODE_EDIT_DIALOG_HPP
#define NODE_EDIT_DIALOG_HPP

#include <gtkmm.h>
#include "MindMap.hpp"
#include <memory>

// Forward declaration
class EditNodeCommand;

class NodeEditDialog : public Gtk::Dialog {
public:
    NodeEditDialog(Gtk::Window& parent, std::shared_ptr<Node> node);

    std::string getNewText() const;
    std::string getNewFont() const;
    Color getNewTextColor() const;
    Color getNewColor() const; // For connection color, returns original if root
    std::string getNewConnText() const;
    
    // Returns validated path. Shows warning dialog if invalid.
    std::string getNewImagePath();
    // Returns validated path. Shows warning dialog if invalid.
    std::string getNewConnImagePath();

    int getNewImgWidth() const;
    int getNewImgHeight() const;

    // Factory method to create an EditNodeCommand from the dialog state
    std::unique_ptr<EditNodeCommand> createEditCommand();

private:
    std::shared_ptr<Node> m_node;

    // Original state for undo
    std::string m_origText;
    std::string m_origFont;
    Color m_origColor;
    Color m_origTextColor;
    std::string m_origImagePath;
    int m_origImgWidth;
    int m_origImgHeight;
    std::string m_origConnText;
    std::string m_origConnImagePath;
    bool m_origOvrC;
    bool m_origOvrT;
    bool m_origOvrF;

    Gtk::Entry m_entryText;
    Gtk::FontButton m_btnFont;
    Gtk::ColorButton m_btnTextColor;
    Gtk::ColorButton m_colorBtnConn;
    
    Gtk::FileChooserButton m_btnImg;
    Gtk::SpinButton m_spinW;
    Gtk::SpinButton m_spinH;

    Gtk::Entry m_entryConnText;
    Gtk::FileChooserButton m_btnConnImg;
    
    // Validation helper
    std::string validateImage(const std::string& path, const std::string& contextName);
};

#endif // NODE_EDIT_DIALOG_HPP
