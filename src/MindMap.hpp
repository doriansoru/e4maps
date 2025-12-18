#ifndef MINDMAP_HPP
#define MINDMAP_HPP

#include <string>
#include <vector>
#include <memory>
#include "Theme.hpp"

// Forward declaration to avoid including tinyxml2.h in header
namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

struct Color {
    double r, g, b;
    static Color random();
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
    std::string connFontDesc;
    bool overrideConnFont = false;

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

    // UI Cache (type-erased to avoid strict dependencies in Model)
    mutable std::shared_ptr<void> _layoutCache;
    
    static int generateId();

    Node(const std::string& t, Color c);

    void addChild(std::shared_ptr<Node> child);

    void removeChild(std::shared_ptr<Node> child);

    bool isRoot() const;

    bool contains(double px, double py) const;
    

    // New tinyxml2-based method for file I/O
    tinyxml2::XMLElement* toXMLElement(tinyxml2::XMLDocument* doc) const;

    static std::shared_ptr<Node> fromXMLElement(tinyxml2::XMLElement* element);
};

class MindMap {
public:
    std::shared_ptr<Node> root;
    Theme theme;

    MindMap(const std::string& rootText);
    
    MindMap();

    std::shared_ptr<Node> hitTest(double x, double y);
    
    void saveToFile(const std::string& filename);
    
    static std::shared_ptr<MindMap> loadFromFile(const std::string& filename);

private:
    std::shared_ptr<Node> hitTestRecursive(std::shared_ptr<Node> node, double x, double y);
};

// Helper to clone tree preserving IDs for layout calculation
std::shared_ptr<Node> cloneNodeTree(std::shared_ptr<Node> original);

#endif // MINDMAP_HPP