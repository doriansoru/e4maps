#ifndef NODE_EDIT_DIALOG_HPP
#define NODE_EDIT_DIALOG_HPP

#include <gtkmm.h>
#include "MindMap.hpp"

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

private:
    std::shared_ptr<Node> m_node;

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
