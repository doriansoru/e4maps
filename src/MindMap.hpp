#ifndef MINDMAP_HPP
#define MINDMAP_HPP

#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Translation.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "tinyxml2.h"
#include "Theme.hpp"

struct Color {
    double r, g, b;
    static Color random() {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return {dis(gen), dis(gen), dis(gen)};
    }
};

class Node : public std::enable_shared_from_this<Node> {
public:
    std::string text;
    std::string fontDesc; // Es: "Sans Bold 14"
    
    std::string imagePath;
    int imgWidth = 0;  // 0 = auto
    int imgHeight = 0; // 0 = auto
    
    std::string connText;
    std::string connImagePath;

    Color color; // Connection color (incoming branch)
    Color textColor = {0.0, 0.0, 0.0}; // Node text color
    
    // Style overrides
    bool overrideColor = false;
    bool overrideTextColor = false;
    bool overrideFont = false;
    
    std::vector<std::shared_ptr<Node>> children;
    std::weak_ptr<Node> parent;
    
    double x = 0.0, y = 0.0;
    double width = 0.0, height = 0.0;
    double angle = 0.0;
    
    bool manualPosition = false;

    // Unique ID for mapping during layout
    int id;
    
    static int generateId() {
        static int _nextId = 0;
        return ++_nextId;
    }

    Node(const std::string& t, Color c) : text(t), color(c), id(generateId()) {
        // Default font
        fontDesc = "Sans Bold 14"; 
    }

    void addChild(std::shared_ptr<Node> child) {
        child->parent = weak_from_this();
        children.push_back(child);
    }

    void removeChild(std::shared_ptr<Node> child) {
        auto it = std::remove(children.begin(), children.end(), child);
        children.erase(it, children.end());
    }

    bool isRoot() const { return parent.expired(); }

    bool contains(double px, double py) const {
        double margin = E4Maps::NODE_MARGIN;
        return (px >= x - width/2 - margin && px <= x + width/2 + margin &&
                py >= y - height/2 - margin && py <= y + height/2 + margin);
    }
    

    // New tinyxml2-based method for file I/O
    tinyxml2::XMLElement* toXMLElement(tinyxml2::XMLDocument* doc) const {
        auto element = doc->NewElement("node");

        element->SetAttribute("text", text.c_str());

        // Only save font attribute if it's being overridden
        if (overrideFont) {
            element->SetAttribute("font", fontDesc.c_str());
        }

        // Only save image attributes if they exist
        if (!imagePath.empty()) {
            element->SetAttribute("img", imagePath.c_str());
        }
        if (imgWidth > 0) {
            element->SetAttribute("iw", imgWidth);
        }
        if (imgHeight > 0) {
            element->SetAttribute("ih", imgHeight);
        }

        element->SetAttribute("ctext", connText.c_str());

        // Only save connection image attribute if it exists
        if (!connImagePath.empty()) {
            element->SetAttribute("cimg", connImagePath.c_str());
        }

        element->SetAttribute("r", (int)(color.r*255));
        element->SetAttribute("g", (int)(color.g*255));
        element->SetAttribute("b", (int)(color.b*255));
        element->SetAttribute("tr", (int)(textColor.r*255));
        element->SetAttribute("tg", (int)(textColor.g*255));
        element->SetAttribute("tb", (int)(textColor.b*255));
        element->SetAttribute("x", x);
        element->SetAttribute("y", y);
        element->SetAttribute("manual", manualPosition ? 1 : 0);

        // Save override flags
        element->SetAttribute("ovr_c", overrideColor ? 1 : 0);
        element->SetAttribute("ovr_t", overrideTextColor ? 1 : 0);
        element->SetAttribute("ovr_f", overrideFont ? 1 : 0);

        // Add child nodes recursively
        for(const auto& child : children) {
            element->InsertEndChild(child->toXMLElement(doc));
        }

        return element;
    }

    static std::shared_ptr<Node> fromXMLElement(tinyxml2::XMLElement* element) {
        if (!element) return nullptr;

        // Get attributes from the XML element
        const char* text = element->Attribute("text");
        const char* font = element->Attribute("font");
        const char* img = element->Attribute("img");
        int iw = element->IntAttribute("iw", 0);
        int ih = element->IntAttribute("ih", 0);
        const char* ctext = element->Attribute("ctext");
        const char* cimg = element->Attribute("cimg");
        int r = element->IntAttribute("r", 0);
        int g = element->IntAttribute("g", 0);
        int b = element->IntAttribute("b", 0);
        int tr = element->IntAttribute("tr", 0);
        int tg = element->IntAttribute("tg", 0);
        int tb = element->IntAttribute("tb", 0);
        double x = element->DoubleAttribute("x", 0.0);
        double y = element->DoubleAttribute("y", 0.0);
        bool manual = element->IntAttribute("manual", 0) == 1;

        // Load override flags with legacy compatibility
        bool ovr_c = false;
        if (element->QueryBoolAttribute("ovr_c", &ovr_c) != tinyxml2::XML_SUCCESS) {
            // Legacy: If ovr_c is missing, we consider it an override ONLY if the color attributes exist.
            if (element->Attribute("r")) ovr_c = true;
        }

        bool ovr_t = false;
        if (element->QueryBoolAttribute("ovr_t", &ovr_t) != tinyxml2::XML_SUCCESS) {
            if (element->Attribute("tr")) ovr_t = true;
        }

        bool ovr_f = false;
        if (element->QueryBoolAttribute("ovr_f", &ovr_f) != tinyxml2::XML_SUCCESS) {
            // Legacy: If ovr_f is missing, we consider it an override ONLY if the font attribute exists.
            if (element->Attribute("font")) ovr_f = true;
        }

        // Create node with the extracted data
        std::string textStr = text ? text : "";

        // For font, set the font string appropriately
        std::string fontStr = "Sans Bold 14";  // Default font for new nodes
        if (ovr_f && font) {
            // Font override is true and font exists in file - use it
            fontStr = font;
        } else if (!ovr_f && font) {
            // Font exists in file but override flag is false (user explicitly unset it)
            // Store the original font value but it won't be used due to override flag = false
            fontStr = font;
        } else if (!ovr_f && !font) {
            // No font in file and no override intended - use default
            fontStr = "Sans Bold 14";
        } else if (ovr_f && !font) {
            // This shouldn't happen in normal scenarios, but for safety use default
            fontStr = "Sans Bold 14";
        }

        std::string imgStr = img ? img : "";
        std::string ctextStr = ctext ? ctext : "";
        std::string cimgStr = cimg ? cimg : "";

        auto node = std::make_shared<Node>(textStr, Color{r/255.0, g/255.0, b/255.0});
        node->textColor = {tr/255.0, tg/255.0, tb/255.0};
        node->fontDesc = fontStr;
        node->imagePath = imgStr;
        node->imgWidth = iw;
        node->imgHeight = ih;
        node->connText = ctextStr;
        node->connImagePath = cimgStr;
        node->x = x;
        node->y = y;
        node->manualPosition = manual;

        node->overrideColor = ovr_c;
        node->overrideTextColor = ovr_t;
        node->overrideFont = ovr_f;

        // Process child elements
        for (tinyxml2::XMLElement* childElement = element->FirstChildElement("node");
             childElement;
             childElement = childElement->NextSiblingElement("node")) {
            auto childNode = Node::fromXMLElement(childElement);
            if (childNode) {
                node->addChild(childNode);
            }
        }

        return node;
    }
};

class MindMap {
public:
    std::shared_ptr<Node> root;
    Theme theme;

    MindMap(const std::string& rootText) {
        root = std::make_shared<Node>(rootText, Color{0.0, 0.0, 0.0});
    }
    
    MindMap() {}

    std::shared_ptr<Node> hitTest(double x, double y) {
        return hitTestRecursive(root, x, y);
    }
    
    void saveToFile(const std::string& filename) {
        if (!root) return;

        tinyxml2::XMLDocument doc;
        
        // Create new root element for the map file
        auto mapElement = doc.NewElement("mindmap");
        doc.InsertFirstChild(mapElement);
        
        // Save Theme
        theme.save(mapElement, &doc);
        
        // Save Nodes
        auto rootElement = root->toXMLElement(&doc);
        mapElement->InsertEndChild(rootElement);

        doc.SaveFile(filename.c_str());
    }
    
    static std::shared_ptr<MindMap> loadFromFile(const std::string& filename) {
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLError result = doc.LoadFile(filename.c_str());
        if (result != tinyxml2::XML_SUCCESS) {
            throw std::runtime_error(_("Cannot open file"));
        }

        auto map = std::make_shared<MindMap>();
        
        // Check root element
        tinyxml2::XMLElement* rootElement = doc.RootElement();
        if (!rootElement) throw std::runtime_error(_("Invalid XML file"));
        
        std::string rootName = rootElement->Name();
        if (rootName == "mindmap") {
            // New format
            // Load Theme
            map->theme.load(rootElement);
            
            // Load Node Tree
            tinyxml2::XMLElement* nodeElement = rootElement->FirstChildElement("node");
            if (nodeElement) {
                map->root = Node::fromXMLElement(nodeElement);
            }
        } else if (rootName == "node") {
            // Old format (Root is the node itself)
            map->root = Node::fromXMLElement(rootElement);
            // Theme remains default
        } else {
             throw std::runtime_error(_("Unknown file format"));
        }
        
        return map;
    }

private:
    std::shared_ptr<Node> hitTestRecursive(std::shared_ptr<Node> node, double x, double y) {
        if (!node) return nullptr;
        if (node->contains(x, y)) return node;
        for (auto& child : node->children) {
            auto found = hitTestRecursive(child, x, y);
            if (found) return found;
        }
        return nullptr;
    }
};

// Helper to clone tree preserving IDs for layout calculation
inline std::shared_ptr<Node> cloneNodeTree(std::shared_ptr<Node> original) {
    if (!original) return nullptr;
    auto copy = std::make_shared<Node>(original->text, original->color);
    // Copy all properties
    copy->id = original->id; // IMPORTANT: Keep same ID for mapping back!
    copy->fontDesc = original->fontDesc;
    copy->textColor = original->textColor;
    copy->imagePath = original->imagePath;
    copy->imgWidth = original->imgWidth;
    copy->imgHeight = original->imgHeight;
    copy->connText = original->connText;
    copy->connImagePath = original->connImagePath;
    copy->x = original->x;
    copy->y = original->y;
    copy->width = original->width;
    copy->height = original->height;
    copy->angle = original->angle;
    copy->manualPosition = original->manualPosition;
    
    // Copy overrides
    copy->overrideColor = original->overrideColor;
    copy->overrideTextColor = original->overrideTextColor;
    copy->overrideFont = original->overrideFont;
    
    for (const auto& child : original->children) {
        auto childCopy = cloneNodeTree(child);
        copy->addChild(childCopy);
    }
    return copy;
}

#endif // MINDMAP_HPP
