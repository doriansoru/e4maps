#ifndef MAPSERIALIZER_HPP
#define MAPSERIALIZER_HPP

#include <string>
#include <memory>
#include "MindMap.hpp"

// Forward declaration to avoid exposing TinyXML2 in the header
namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

class MapSerializer {
public:
    /**
     * Saves the entire mind map to a file.
     * @param map The mind map to save.
     * @param filename The destination file path.
     */
    static void save(std::shared_ptr<MindMap> map, const std::string& filename);

    /**
     * Loads a mind map from a file.
     * @param filename The source file path.
     * @return A shared pointer to the loaded MindMap.
     */
    static std::shared_ptr<MindMap> load(const std::string& filename);

private:
    // Helper to convert a single Node to XML
    static tinyxml2::XMLElement* nodeToXml(const Node* node, tinyxml2::XMLDocument* doc);

    // Helper to create a Node from XML
    static std::shared_ptr<Node> xmlToNode(tinyxml2::XMLElement* element);
};

#endif // MAPSERIALIZER_HPP