#include "ThemeEditor.hpp"
#include "Translation.hpp"
#include <iostream>

// Helper: Cairo::Pattern to Gdk::RGBA
static Gdk::RGBA patternToRGBA(const Cairo::RefPtr<Cairo::Pattern>& pat) {
    Gdk::RGBA color;
    auto solid = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(pat);
    if (solid) {
        double r, g, b, a;
        solid->get_rgba(r, g, b, a);
        color.set_rgba(r, g, b, a);
    } else {
        color.set_rgba(0, 0, 0, 1);
    }
    return color;
}

// Helper: Gdk::RGBA to Cairo::Pattern
static Cairo::RefPtr<Cairo::Pattern> rgbaToPattern(const Gdk::RGBA& color) {
    return Cairo::SolidPattern::create_rgba(color.get_red(), color.get_green(), color.get_blue(), color.get_alpha());
}

ThemeEditor::ThemeEditor(Gtk::Window& parent, Theme& theme)
    : Gtk::Dialog(_("Theme Editor"), parent, true),
      m_workingTheme(theme),
      m_mainBox(Gtk::ORIENTATION_VERTICAL),
      m_selectorBox(Gtk::ORIENTATION_VERTICAL),
      m_radioLevel(_("Level"))
{
    // m_radioClass.join_group(m_radioLevel); // Removed


    set_default_size(800, 600);
    
    // Main layout
    get_content_area()->pack_start(m_mainBox, Gtk::PACK_EXPAND_WIDGET);
    
    // Selector Box Setup (Top Left Area replacement)
    m_selectorBox.set_border_width(10);
    m_selectorBox.set_spacing(10);

    // Level Row
    Gtk::Box* levelRow = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    levelRow->set_spacing(10);
    levelRow->pack_start(m_radioLevel, Gtk::PACK_SHRINK);
    
    m_spinLevel.set_adjustment(Gtk::Adjustment::create(0.0, 0.0, 9999.0, 1.0)); // Infinite enough
    m_spinLevel.set_digits(0);
    m_spinLevel.set_numeric(true);
    levelRow->pack_start(m_spinLevel, Gtk::PACK_EXPAND_WIDGET);
    
    m_selectorBox.pack_start(*levelRow, Gtk::PACK_SHRINK);

    // Paned Layout
    m_paned.set_position(250); // Give a bit more space for the selector
    m_mainBox.pack_start(m_paned, Gtk::PACK_EXPAND_WIDGET);

    // Left: Selector Box (wrapped in a Frame or just Box)
    // We want it to be at the top, maybe? Or just the left pane.
    // The previous design had a scrolled list. Now we have a small control area.
    // To make it look good, let's put it in a Frame and align it to top.
    Gtk::Frame* leftFrame = Gtk::manage(new Gtk::Frame(_("Edit Style For:")));
    Gtk::Box* leftContainer = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    leftFrame->add(*leftContainer);
    leftContainer->pack_start(m_selectorBox, Gtk::PACK_SHRINK);
    
    // Add a label explaining inheritance?
    Gtk::Label* hintLabel = Gtk::manage(new Gtk::Label(_("Levels inherit from their\nclosest defined parent.\nModify to override."), Gtk::ALIGN_START));
    hintLabel->set_line_wrap(true);
    hintLabel->set_margin_left(10);
    hintLabel->set_margin_right(10);
    hintLabel->set_margin_top(20);
    leftContainer->pack_start(*hintLabel, Gtk::PACK_SHRINK);

    m_paned.add1(*leftFrame);

    // Right: Properties
    m_propScroll.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_propBox.set_orientation(Gtk::ORIENTATION_VERTICAL);
    m_propBox.set_margin_left(20);
    m_propBox.set_margin_right(20);
    m_propBox.set_margin_top(20);
    m_propBox.set_margin_bottom(20);
    
    Gtk::Widget* grid = createPropertyGrid();
    m_propBox.pack_start(*grid, Gtk::PACK_SHRINK);
    m_propScroll.add(m_propBox);
    m_paned.add2(m_propScroll);
    
    // Connect signals and store connections
    m_radioLevelConnection = m_radioLevel.signal_toggled().connect(sigc::mem_fun(*this, &ThemeEditor::onModeChanged));
    m_spinLevelConnection = m_spinLevel.signal_value_changed().connect(sigc::mem_fun(*this, &ThemeEditor::onLevelChanged));

    add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    add_button(_("OK"), Gtk::RESPONSE_OK);

    show_all_children();

    // Initialize State
    m_selectedLevel = 0;
    m_isLevelSelected = true;
    m_radioLevel.set_active(true);
    m_spinLevel.set_value(0);

    loadStyleProperties(m_workingTheme.getStyle(0));

}



void ThemeEditor::onModeChanged() {
    // With only level mode, this function is mostly vestigial but kept for structure.
    // If we only have m_radioLevel, it's always active, so no actual mode change.
    // However, if there were other radio buttons that could disable m_radioLevel,
    // the logic to save and reload would be here.
    // For now, it just means ensure previous edits are saved if this was triggered.
    if (m_isBeingDestroyed) return;
    saveCurrentStyle();

    m_isLevelSelected = true; // Always level selected
    m_spinLevel.set_sensitive(true); // Always sensitive
    // m_comboClass removed

    m_selectedLevel = m_spinLevel.get_value_as_int();
    loadStyleProperties(m_workingTheme.getStyle(m_selectedLevel));
}

void ThemeEditor::onLevelChanged() {
    if (!m_isLevelSelected || m_isBeingDestroyed) return;
    saveCurrentStyle();

    m_selectedLevel = m_spinLevel.get_value_as_int();
    // getStyle returns the style for this level if exists, or inherited if not.
    loadStyleProperties(m_workingTheme.getStyle(m_selectedLevel));
}



Gtk::Widget* ThemeEditor::createPropertyGrid() {
    Gtk::Grid* grid = Gtk::manage(new Gtk::Grid());
    grid->set_row_spacing(10);
    grid->set_column_spacing(15);
    
    int row = 0;
    
    // Section: Appearance
    grid->attach(*Gtk::manage(new Gtk::Label("<b>" + Glib::ustring(_("Appearance")) + "</b>")),
                 0, row++, 2, 1);
    static_cast<Gtk::Label*>(grid->get_child_at(0, row-1))->set_use_markup(true);

    m_btnBgColor.set_title(_("Background Color"));
    addProperty(*grid, row++, _("Background:"), m_btnBgColor);
    
    m_btnBgHoverColor.set_title(_("Hover Color"));
    addProperty(*grid, row++, _("Hover Background:"), m_btnBgHoverColor);

    m_btnBorderColor.set_title(_("Border Color"));
    addProperty(*grid, row++, _("Border Color:"), m_btnBorderColor);
    
    m_spinBorderWidth.set_adjustment(Gtk::Adjustment::create(1.0, 0.0, 20.0, 0.5));
    m_spinBorderWidth.set_digits(1);
    addProperty(*grid, row++, _("Border Width:"), m_spinBorderWidth);

    m_spinCornerRadius.set_adjustment(Gtk::Adjustment::create(5.0, 0.0, 50.0, 1.0));
    m_spinCornerRadius.set_digits(1);
    addProperty(*grid, row++, _("Corner Radius:"), m_spinCornerRadius);
    
    // Section: Text
    grid->attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, row++, 2, 1);
    grid->attach(*Gtk::manage(new Gtk::Label("<b>" + Glib::ustring(_("Text")) + "</b>")),
                 0, row++, 2, 1);
    static_cast<Gtk::Label*>(grid->get_child_at(0, row-1))->set_use_markup(true);
    
    m_btnFont.set_title(_("Font"));
    addProperty(*grid, row++, _("Font:"), m_btnFont);
    
    m_btnTextColor.set_title(_("Text Color"));
    addProperty(*grid, row++, _("Text Color:"), m_btnTextColor);
    
    // Section: Layout & Shadow
    grid->attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, row++, 2, 1);
    // Escape text to handle '&' characters correctly in markup
    grid->attach(*Gtk::manage(new Gtk::Label("<b>" + Glib::Markup::escape_text(_("Layout & Shadow")) + "</b>")),
                 0, row++, 2, 1);
    static_cast<Gtk::Label*>(grid->get_child_at(0, row-1))->set_use_markup(true);
    
    m_spinPadH.set_adjustment(Gtk::Adjustment::create(10.0, 0.0, 100.0, 1.0));
    addProperty(*grid, row++, _("Horizontal Padding:"), m_spinPadH);
    
    m_spinPadV.set_adjustment(Gtk::Adjustment::create(5.0, 0.0, 100.0, 1.0));
    addProperty(*grid, row++, _("Vertical Padding:"), m_spinPadV);
    
    m_btnShadowColor.set_title(_("Shadow Color"));
    addProperty(*grid, row++, _("Shadow Color:"), m_btnShadowColor);
    
    // Section: Connections
    grid->attach(*Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)), 0, row++, 2, 1);
    grid->attach(*Gtk::manage(new Gtk::Label("<b>" + Glib::ustring(_("Connections (To Children)")) + "</b>")),
                 0, row++, 2, 1);
    static_cast<Gtk::Label*>(grid->get_child_at(0, row-1))->set_use_markup(true);
    
    m_btnConnColor.set_title(_("Line Color"));
    addProperty(*grid, row++, _("Line Color:"), m_btnConnColor);
    
    m_spinConnWidth.set_adjustment(Gtk::Adjustment::create(1.0, 0.1, 10.0, 0.5));
    m_spinConnWidth.set_digits(1);
    addProperty(*grid, row++, _("Line Width:"), m_spinConnWidth);
    
    addProperty(*grid, row++, _("Dashed Line:"), m_checkConnDash);

    return grid;
}

void ThemeEditor::addProperty(Gtk::Grid& grid, int row, const Glib::ustring& label, Gtk::Widget& widget) {
    Gtk::Label* lbl = Gtk::manage(new Gtk::Label(label, Gtk::ALIGN_START));
    grid.attach(*lbl, 0, row, 1, 1);
    grid.attach(widget, 1, row, 1, 1);
    widget.set_hexpand(true);
}

void ThemeEditor::loadStyleProperties(const NodeStyle& style) {
    // Don't load if the window is being destroyed
    if (m_isBeingDestroyed) {
        return;
    }

    m_btnBgColor.set_rgba(patternToRGBA(style.backgroundColor));
    m_btnBgHoverColor.set_rgba(patternToRGBA(style.backgroundHoverColor));
    m_btnBorderColor.set_rgba(patternToRGBA(style.borderColor));
    m_spinBorderWidth.set_value(style.borderWidth);

    m_btnShadowColor.set_rgba(patternToRGBA(style.shadowColor));

    m_btnFont.set_font_name(style.fontDescription.to_string());
    m_btnTextColor.set_rgba(patternToRGBA(style.textColor));

    m_spinCornerRadius.set_value(style.cornerRadius);
    m_spinPadH.set_value(style.horizontalPadding);
    m_spinPadV.set_value(style.verticalPadding);

    m_btnConnColor.set_rgba(patternToRGBA(style.connectionColor));
    m_spinConnWidth.set_value(style.connectionWidth);
    m_checkConnDash.set_active(style.connectionDash);
}

void ThemeEditor::saveCurrentStyle() {
    // Don't save if the window is being destroyed
    if (m_isBeingDestroyed) {
        return;
    }

    NodeStyle* style = nullptr;

    // Always saving for the currently selected level, as class styles are removed
    // Handle Map insertion logic to preserve inheritance
    auto& levels = m_workingTheme.getLevelStyles();
    if (levels.find(m_selectedLevel) == levels.end()) {
         // Create it as a copy of inherited
         // MUST call getStyle BEFORE inserting, otherwise getStyle finds the empty new insertion!
         NodeStyle inherited = m_workingTheme.getStyle(m_selectedLevel);
         levels[m_selectedLevel] = inherited;
    }
    style = &levels[m_selectedLevel];

    if (style) {
        style->backgroundColor = rgbaToPattern(m_btnBgColor.get_rgba());
        style->backgroundHoverColor = rgbaToPattern(m_btnBgHoverColor.get_rgba());
        style->borderColor = rgbaToPattern(m_btnBorderColor.get_rgba());
        style->borderWidth = m_spinBorderWidth.get_value();
        style->shadowColor = rgbaToPattern(m_btnShadowColor.get_rgba());
        style->fontDescription = Pango::FontDescription(m_btnFont.get_font_name());
        style->textColor = rgbaToPattern(m_btnTextColor.get_rgba());
        style->cornerRadius = m_spinCornerRadius.get_value();
        style->horizontalPadding = m_spinPadH.get_value();
        style->verticalPadding = m_spinPadV.get_value();
        style->connectionColor = rgbaToPattern(m_btnConnColor.get_rgba());
        style->connectionWidth = m_spinConnWidth.get_value();
        style->connectionDash = m_checkConnDash.get_active();
    }
}

void ThemeEditor::on_response(int response_id) {
    if (response_id == Gtk::RESPONSE_OK) {
        saveCurrentStyle(); // Ensure current edits are saved before closing
    }

    // Set the flag to prevent further operations on widgets during destruction
    m_isBeingDestroyed = true;

    // Disconnect all signals to prevent callbacks during destruction
    if (m_radioLevelConnection) {
        m_radioLevelConnection.disconnect();
    }
    if (m_spinLevelConnection) {
        m_spinLevelConnection.disconnect();
    }

    // Call base class implementation to close the dialog
    Gtk::Dialog::on_response(response_id);
}
