#include "Theme.hpp"
#include <pangomm.h> // Ensure Pango::SCALE is available
#include <iostream>

// Helper to serialize Pattern to Hex
static std::string patternToHex(const Cairo::RefPtr<Cairo::Pattern>& pat) {
    if (!pat) return "#000000";
    auto solid = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(pat);
    if (solid) {
        double r, g, b, a;
        solid->get_rgba(r, g, b, a);
        return cairoToHex(r, g, b, a);
    }
    return "#000000"; // Fallback for non-solid patterns
}

// Helper to deserialize Hex to Pattern
static Cairo::RefPtr<Cairo::Pattern> hexToPattern(const std::string& hex) {
    double r, g, b, a;
    hexToCairo(hex, r, g, b, a);
    return Cairo::SolidPattern::create_rgba(r, g, b, a);
}

// NodeStyle default constructor
NodeStyle::NodeStyle()
    : borderWidth(1.0),
      shadowOffsetX(2.0),
      shadowOffsetY(2.0),
      shadowBlurRadius(5.0),
      cornerRadius(5.0),
      horizontalPadding(10.0),
      verticalPadding(5.0),
      connectionWidth(1.0),
      connectionDash(false)
{
    // Default colors and patterns (can be overridden by theme or specific styles)
    backgroundColor = Cairo::SolidPattern::create_rgb(0.9, 0.9, 0.9); // Light gray
    backgroundHoverColor = Cairo::SolidPattern::create_rgb(0.8, 0.8, 0.8); // Slightly darker gray
    borderColor = Cairo::SolidPattern::create_rgb(0.2, 0.2, 0.2); // Dark gray
    shadowColor = Cairo::SolidPattern::create_rgba(0.0, 0.0, 0.0, 0.5); // Semi-transparent black
    textColor = Cairo::SolidPattern::create_rgb(0.0, 0.0, 0.0); // Black
    connectionColor = Cairo::SolidPattern::create_rgb(0.5, 0.5, 0.5); // Gray

    fontDescription.set_family("Sans");
    fontDescription.set_weight(Pango::WEIGHT_NORMAL);
    fontDescription.set_size(12 * Pango::SCALE); // 12pt
}

tinyxml2::XMLElement* NodeStyle::toXMLElement(tinyxml2::XMLDocument* doc, const std::string& elementName) const {
    auto el = doc->NewElement(elementName.c_str());
    
    el->SetAttribute("bg", patternToHex(backgroundColor).c_str());
    el->SetAttribute("bg_hover", patternToHex(backgroundHoverColor).c_str());
    el->SetAttribute("border", patternToHex(borderColor).c_str());
    el->SetAttribute("border_w", borderWidth);
    el->SetAttribute("shadow", patternToHex(shadowColor).c_str());
    el->SetAttribute("shadow_off_x", shadowOffsetX);
    el->SetAttribute("shadow_off_y", shadowOffsetY);
    el->SetAttribute("shadow_blur", shadowBlurRadius);
    el->SetAttribute("font", fontDescription.to_string().c_str());
    el->SetAttribute("text_color", patternToHex(textColor).c_str());
    el->SetAttribute("corner_r", cornerRadius);
    el->SetAttribute("pad_h", horizontalPadding);
    el->SetAttribute("pad_v", verticalPadding);
    el->SetAttribute("conn_color", patternToHex(connectionColor).c_str());
    el->SetAttribute("conn_w", connectionWidth);
    el->SetAttribute("conn_dash", connectionDash);

    return el;
}

NodeStyle NodeStyle::fromXMLElement(tinyxml2::XMLElement* element) {
    NodeStyle style;
    if (!element) return style;

    if (element->Attribute("bg")) style.backgroundColor = hexToPattern(element->Attribute("bg"));
    if (element->Attribute("bg_hover")) style.backgroundHoverColor = hexToPattern(element->Attribute("bg_hover"));
    if (element->Attribute("border")) style.borderColor = hexToPattern(element->Attribute("border"));
    if (element->Attribute("border_w")) style.borderWidth = element->DoubleAttribute("border_w");
    
    if (element->Attribute("shadow")) style.shadowColor = hexToPattern(element->Attribute("shadow"));
    if (element->Attribute("shadow_off_x")) style.shadowOffsetX = element->DoubleAttribute("shadow_off_x");
    if (element->Attribute("shadow_off_y")) style.shadowOffsetY = element->DoubleAttribute("shadow_off_y");
    if (element->Attribute("shadow_blur")) style.shadowBlurRadius = element->DoubleAttribute("shadow_blur");
    
    if (element->Attribute("font")) style.fontDescription = Pango::FontDescription(element->Attribute("font"));
    if (element->Attribute("text_color")) style.textColor = hexToPattern(element->Attribute("text_color"));
    
    if (element->Attribute("corner_r")) style.cornerRadius = element->DoubleAttribute("corner_r");
    if (element->Attribute("pad_h")) style.horizontalPadding = element->DoubleAttribute("pad_h");
    if (element->Attribute("pad_v")) style.verticalPadding = element->DoubleAttribute("pad_v");
    
    if (element->Attribute("conn_color")) style.connectionColor = hexToPattern(element->Attribute("conn_color"));
    if (element->Attribute("conn_w")) style.connectionWidth = element->DoubleAttribute("conn_w");
    if (element->Attribute("conn_dash")) style.connectionDash = element->BoolAttribute("conn_dash");

    return style;
}

// Theme constructor
Theme::Theme() : name("Default") {
    initializeDefaultStyles();
}

void Theme::save(tinyxml2::XMLElement* root, tinyxml2::XMLDocument* doc) const {
    auto themeEl = doc->NewElement("theme");
    themeEl->SetAttribute("name", name.c_str());
    
    auto levelsEl = doc->NewElement("level_styles");
    for (const auto& pair : levelStyles) {
        auto styleEl = pair.second.toXMLElement(doc, "style");
        styleEl->SetAttribute("level", pair.first);
        levelsEl->InsertEndChild(styleEl);
    }
    themeEl->InsertEndChild(levelsEl);
    
    root->InsertEndChild(themeEl);
}

void Theme::load(tinyxml2::XMLElement* root) {
    auto themeEl = root->FirstChildElement("theme");
    if (!themeEl) return;
    
    if (themeEl->Attribute("name")) {
        name = themeEl->Attribute("name");
    }
    
    levelStyles.clear();
    
    auto levelsEl = themeEl->FirstChildElement("level_styles");
    if (levelsEl) {
        for (auto styleEl = levelsEl->FirstChildElement("style"); styleEl; styleEl = styleEl->NextSiblingElement("style")) {
            int level = styleEl->IntAttribute("level", -1);
            if (level >= 0) {
                levelStyles[level] = NodeStyle::fromXMLElement(styleEl);
            }
        }
    }
    
    // Fallback if empty
    if (levelStyles.empty()) {
        initializeDefaultStyles();
    }
}

void Theme::initializeDefaultStyles() {
    // Level 0 (Root Node)
    NodeStyle rootStyle;
    rootStyle.backgroundColor = Cairo::SolidPattern::create_rgb(0.8, 0.8, 0.9); // Light blueish
    rootStyle.borderColor = Cairo::SolidPattern::create_rgb(0.4, 0.4, 0.6);
    rootStyle.fontDescription.set_weight(Pango::WEIGHT_BOLD);
    rootStyle.fontDescription.set_size(18 * Pango::SCALE);
    rootStyle.horizontalPadding = 20.0;
    rootStyle.verticalPadding = 10.0;
    levelStyles[0] = rootStyle;

    // Level 1 (First level children)
    NodeStyle level1Style;
    level1Style.backgroundColor = Cairo::SolidPattern::create_rgb(0.9, 0.9, 0.8); // Light yellowish
    level1Style.borderColor = Cairo::SolidPattern::create_rgb(0.6, 0.6, 0.4);
    level1Style.fontDescription.set_weight(Pango::WEIGHT_BOLD);
    level1Style.fontDescription.set_size(14 * Pango::SCALE);
    level1Style.horizontalPadding = 15.0;
    level1Style.verticalPadding = 7.0;
    levelStyles[1] = level1Style;

    // Level 2 (and deeper default)
    NodeStyle defaultLevelStyle;
    defaultLevelStyle.backgroundColor = Cairo::SolidPattern::create_rgb(0.95, 0.95, 0.95); // Very light gray
    defaultLevelStyle.borderColor = Cairo::SolidPattern::create_rgb(0.7, 0.7, 0.7);
    defaultLevelStyle.fontDescription.set_weight(Pango::WEIGHT_NORMAL);
    defaultLevelStyle.fontDescription.set_size(12 * Pango::SCALE);
    defaultLevelStyle.fontDescription.set_style(Pango::STYLE_ITALIC);
    levelStyles[2] = defaultLevelStyle;
}

NodeStyle Theme::getStyle(int level) const {
    // Find the style for this level, or the nearest defined parent level.
    // std::map::upper_bound(key) returns iterator to first element with key > k
    
    if (levelStyles.empty()) return NodeStyle();

    auto it = levelStyles.upper_bound(level);
    
    if (it == levelStyles.begin()) {
        // Requested level is smaller than the smallest key?
        // Should not happen if level 0 is always there.
        // Return the first available style.
        return it->second; 
    }
    
    // Decrement to find the element <= level
    --it;
    return it->second;
}
