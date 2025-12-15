#ifndef E4MAPS_THEME_HPP
#define E4MAPS_THEME_HPP

#include <string>
#include <vector>
#include <map>
#include <cairomm/cairomm.h>
#include <pangomm/fontdescription.h>
#include "tinyxml2.h"
#include "Utils.hpp"

/**
 * @brief Represents the style attributes for a mind map node.
 */
struct NodeStyle {
    // Node background colors
    Cairo::RefPtr<Cairo::Pattern> backgroundColor;
    Cairo::RefPtr<Cairo::Pattern> backgroundHoverColor;

    // Node border
    Cairo::RefPtr<Cairo::Pattern> borderColor;
    double borderWidth;

    // Node shadow
    Cairo::RefPtr<Cairo::Pattern> shadowColor;
    double shadowOffsetX;
    double shadowOffsetY;
    double shadowBlurRadius;

    // Text properties
    Pango::FontDescription fontDescription;
    Pango::FontDescription connectionFontDescription;
    Cairo::RefPtr<Cairo::Pattern> textColor;

    // Node shape and padding
    double cornerRadius;
    double horizontalPadding;
    double verticalPadding;

    // Connection properties
    Cairo::RefPtr<Cairo::Pattern> connectionColor;
    double connectionWidth;
    bool connectionDash;

    NodeStyle(); // Default constructor
    
    // Serialization
    tinyxml2::XMLElement* toXMLElement(tinyxml2::XMLDocument* doc, const std::string& elementName) const;
    static NodeStyle fromXMLElement(tinyxml2::XMLElement* element);
};

/**
 * @brief Manages themes for mind map nodes.
 *
 * A theme consists of a collection of NodeStyles, typically indexed by hierarchy level,
 * and can also include styles for specific classes.
 */
class Theme {
public:
    Theme();

    /**
     * @brief Get the NodeStyle for a given hierarchy level.
     * @param level The hierarchy level (0 for root, 1 for children, etc.).
     * @return The NodeStyle for the specified level.
     */
    NodeStyle getStyle(int level) const;
    
    // Theme Management
    void setName(const std::string& n) { name = n; }
    std::string getName() const { return name; }
    
    // Access for Editor
    std::map<int, NodeStyle>& getLevelStyles() { return levelStyles; }
    const std::map<int, NodeStyle>& getLevelStyles() const { return levelStyles; } // Const overload
    
    // Serialization
    void save(tinyxml2::XMLElement* root, tinyxml2::XMLDocument* doc) const;
    void load(tinyxml2::XMLElement* root);

private:
    std::string name;
    std::map<int, NodeStyle> levelStyles;

    // Helper to initialize default styles
    void initializeDefaultStyles();
};

#endif // E4MAPS_THEME_HPP
