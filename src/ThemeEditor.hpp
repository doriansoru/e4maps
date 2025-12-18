#ifndef THEME_EDITOR_HPP
#define THEME_EDITOR_HPP

#include <gtkmm.h>
#include "Theme.hpp"

class ThemeEditor : public Gtk::Dialog {
public:
    ThemeEditor(Gtk::Window& parent, Theme& theme);
    Theme getResult() const { return m_workingTheme; }

private:
    Theme m_workingTheme; // Copy of the theme to edit
    bool m_isBeingDestroyed = false; // Flag to track if window is being destroyed

    // Signal connections to manage during destruction
    sigc::connection m_radioLevelConnection;
    sigc::connection m_spinLevelConnection;
    sigc::connection m_hideConnection;

    // UI Components
    Gtk::Box m_mainBox;
    Gtk::Paned m_paned;
    
    // Selector Area (Left Side)
    Gtk::Box m_selectorBox;
    Gtk::RadioButton m_radioLevel;
    Gtk::SpinButton m_spinLevel;
    
    // Property Area (Right Side)
    Gtk::ScrolledWindow m_propScroll;
    Gtk::Box m_propBox;

    // Property Widgets
    Gtk::ColorButton m_btnBgColor;
    Gtk::ColorButton m_btnBgHoverColor;
    Gtk::ColorButton m_btnBorderColor;
    Gtk::SpinButton m_spinBorderWidth;
    Gtk::ColorButton m_btnShadowColor;
    Gtk::SpinButton m_spinShadowOffX;
    Gtk::SpinButton m_spinShadowOffY;
    Gtk::SpinButton m_spinShadowBlur;
    Gtk::FontButton m_btnFont;
    Gtk::FontButton m_btnConnFont;
    Gtk::ColorButton m_btnTextColor;
    Gtk::ColorButton m_btnTextHoverColor;
    Gtk::SpinButton m_spinCornerRadius;
    Gtk::SpinButton m_spinPadH;
    Gtk::SpinButton m_spinPadV;
    Gtk::ColorButton m_btnConnColor;
    Gtk::SpinButton m_spinConnWidth;
    Gtk::CheckButton m_checkConnDash;
    Gtk::ComboBoxText m_comboConnType;

    // Selection State
    int m_selectedLevel = -1;
    bool m_isLevelSelected = true; // Default to Level mode

    void loadStyleProperties(const NodeStyle& style);
    void saveCurrentStyle(); // Save from widgets to m_workingTheme
    
    // Logic to handle mode switching
    void onModeChanged();
    void onLevelChanged();

protected:
    void on_response(int response_id) override;

    // Helpers
    void addProperty(Gtk::Grid& grid, int row, const Glib::ustring& label, Gtk::Widget& widget);
    Gtk::Widget* createPropertyGrid();
};

#endif // THEME_EDITOR_HPP