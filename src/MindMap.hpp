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
    
    std::string toXML(int indentLevel = 0) {
        std::stringstream ss;
        std::string indent(indentLevel * 2, ' ');

        ss << indent << "<node text=\"" << escapeXml(text) << "\" "
           << "font=\"" << escapeXml(fontDesc) << "\" "
           << "img=\"" << escapeXml(imagePath) << "\" "
           << "iw=\"" << imgWidth << "\" ih=\"" << imgHeight << "\" "
           << "ctext=\"" << escapeXml(connText) << "\" "
           << "cimg=\"" << escapeXml(connImagePath) << "\" "
           << "r=\"" << (int)(color.r*255) << "\" g=\"" << (int)(color.g*255) << "\" b=\"" << (int)(color.b*255) << "\" "
           << "tr=\"" << (int)(textColor.r*255) << "\" tg=\"" << (int)(textColor.g*255) << "\" tb=\"" << (int)(textColor.b*255) << "\" "
           << "x=\"" << x << "\" y=\"" << y << "\" manual=\"" << manualPosition << "\">\n";

        for(auto& child : children) {
            ss << child->toXML(indentLevel + 1);
        }

        ss << indent << "</node>\n";
        return ss.str();
    }
};

class MindMap {
public:
    std::shared_ptr<Node> root;

    MindMap(const std::string& rootText) {
        root = std::make_shared<Node>(rootText, Color{0.0, 0.0, 0.0});
    }
    
    MindMap() {}

    std::shared_ptr<Node> hitTest(double x, double y) {
        return hitTestRecursive(root, x, y);
    }
    
    void saveToFile(const std::string& filename) {
        std::ofstream out(filename);
        if (root) out << root->toXML();
        out.close();
    }
    
    static std::shared_ptr<MindMap> loadFromFile(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) throw std::runtime_error(_("Cannot open file"));
        
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string content = buffer.str();
        
        auto map = std::make_shared<MindMap>();
        size_t pos = 0;
        map->root = parseNode(content, pos);
        return map;
    }

private:
    static int safeStoi(const std::string& s, int def = 0) {
        if (s.empty()) return def;
        try { return std::stoi(s); } catch (...) { return def; }
    }

    static double safeStod(std::string s, double def = 0.0) {
        if (s.empty()) return def;
        std::replace(s.begin(), s.end(), ',', '.');
        try { return std::stod(s); } catch (...) { return def; }
    }

    static std::shared_ptr<Node> parseNode(const std::string& xml, size_t& pos) {
        size_t start = xml.find("<node", pos);
        if (start == std::string::npos) return nullptr;
        pos = start;
        
        std::string text = getAttr(xml, pos, "text");
        std::string font = getAttr(xml, pos, "font");
        if (font.empty()) font = "Sans Bold 14"; // Default if missing

        std::string img = getAttr(xml, pos, "img");
        int iw = safeStoi(getAttr(xml, pos, "iw"), 0);
        int ih = safeStoi(getAttr(xml, pos, "ih"), 0);

        std::string ctext = getAttr(xml, pos, "ctext");
        std::string cimg = getAttr(xml, pos, "cimg");
        
        double r = safeStoi(getAttr(xml, pos, "r")) / 255.0;
        double g = safeStoi(getAttr(xml, pos, "g")) / 255.0;
        double b = safeStoi(getAttr(xml, pos, "b")) / 255.0;
        
        // Text Color (default black if not present)
        double tr = safeStoi(getAttr(xml, pos, "tr")) / 255.0;
        double tg = safeStoi(getAttr(xml, pos, "tg")) / 255.0;
        double tb = safeStoi(getAttr(xml, pos, "tb")) / 255.0;
        
        double x = safeStod(getAttr(xml, pos, "x"));
        double y = safeStod(getAttr(xml, pos, "y"));
        
        bool manual = safeStoi(getAttr(xml, pos, "manual"), 0) == 1;
        
        auto node = std::make_shared<Node>(text, Color{r,g,b});
        node->textColor = {tr, tg, tb};
        node->fontDesc = font;
        node->imagePath = img;
        node->imgWidth = iw; node->imgHeight = ih;
        node->connText = ctext;
        node->connImagePath = cimg;
        node->x = x; node->y = y; node->manualPosition = manual;
        
        size_t tagEnd = xml.find(">", pos);
        pos = tagEnd + 1;
        
        while (true) {
            size_t nextTag = xml.find("<", pos);
            if (nextTag == std::string::npos) break;
            
            if (xml.substr(nextTag, 7) == "</node>") {
                pos = nextTag + 7;
                break;
            } else {
                auto child = parseNode(xml, pos);
                if (child) node->addChild(child);
            }
        }
        return node;
    }
    
    static std::string getAttr(const std::string& xml, size_t start, const std::string& attrName) {
        std::string search = attrName + "=\"" ;
        size_t attrStart = xml.find(search, start);
        if (attrStart == std::string::npos) return "";
        size_t tagClose = xml.find(">", start);
        if (attrStart > tagClose) return ""; 
        attrStart += search.length();
        size_t attrEnd = xml.find("\"", attrStart);
        if (attrEnd == std::string::npos) return "";
        return xml.substr(attrStart, attrEnd - attrStart);
    }

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
    
    for (const auto& child : original->children) {
        auto childCopy = cloneNodeTree(child);
        copy->addChild(childCopy);
    }
    return copy;
}

#endif // MINDMAP_HPP
