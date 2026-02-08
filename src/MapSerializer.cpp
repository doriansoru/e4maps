#include "MapSerializer.hpp"
#include "XmlConstants.hpp"
#include "Translation.hpp"
#include "tinyxml2.h"
#include <stdexcept>
#include <map>
#include <iostream>

using namespace E4Maps::Xml;

namespace {

// Helper function to assign sequential IDs to all nodes in the tree for serialization
int assignSequentialIds(std::shared_ptr<Node> node, int& currentId) {
    if (!node) return currentId;

    node->id = currentId++;

    for (auto& child : node->children) {
        assignSequentialIds(child, currentId);
    }

    return currentId;
}

// Helper function to find a node by its sequential ID
std::shared_ptr<Node> findNodeById(std::shared_ptr<Node> node, int targetId) {
    if (!node) return nullptr;

    if (node->id == targetId) {
        return node;
    }

    for (auto& child : node->children) {
        auto result = findNodeById(child, targetId);
        if (result) return result;
    }

    return nullptr;
}

// Helper function to build a map of node IDs to their traversal index
void buildNodeIndexMap(std::shared_ptr<Node> node, std::map<int, int>& idToIndex, int& currentIndex) {
    if (!node) return;

    idToIndex[node->id] = currentIndex++;

    for (auto& child : node->children) {
        buildNodeIndexMap(child, idToIndex, currentIndex);
    }
}

// Helper function to find the index of a node in the tree for serialization
int findNodeIndex(std::shared_ptr<Node> root, std::shared_ptr<Node> target) {
    if (!root || !target) return -1;

    std::map<int, int> idToIndex;
    int currentIndex = 0;

    buildNodeIndexMap(root, idToIndex, currentIndex);

    auto it = idToIndex.find(target->id);
    if (it != idToIndex.end()) {
        return it->second;
    }

    return -1; // Node not found
}

} // namespace

void MapSerializer::save(std::shared_ptr<MindMap> map, const std::string& filename) {
    if (!map || !map->root) return;

    tinyxml2::XMLDocument doc;

    // Create new root element for the map file
    auto mapElement = doc.NewElement(TAG_MAP);
    doc.InsertFirstChild(mapElement);

    // Save Theme
    map->theme.save(mapElement, &doc);

    // Save Connections
    auto connectionsElement = doc.NewElement(TAG_CONNECTIONS);
    for (const auto& conn : map->connections) {
        if (!conn) continue;
        auto connElement = doc.NewElement(TAG_CONNECTION);

        // Find indices of from and to nodes to store references
        int fromIndex = findNodeIndex(map->root, conn->from);
        int toIndex = findNodeIndex(map->root, conn->to);

        if (fromIndex != -1 && toIndex != -1) {
            connElement->SetAttribute(ATTR_FROM, fromIndex);
            connElement->SetAttribute(ATTR_TO, toIndex);

            // Save connection properties
            if (!conn->text.empty()) {
                connElement->SetAttribute(ATTR_TEXT, conn->text.c_str());
            }
            if (!conn->imagePath.empty()) {
                connElement->SetAttribute(ATTR_IMG, conn->imagePath.c_str());
            }
            connElement->SetAttribute(ATTR_R, (int)(conn->color.r*255));
            connElement->SetAttribute(ATTR_G, (int)(conn->color.g*255));
            connElement->SetAttribute(ATTR_B, (int)(conn->color.b*255));

            if (conn->overrideFont && !conn->fontDesc.empty()) {
                connElement->SetAttribute(ATTR_FONT, conn->fontDesc.c_str());
                connElement->SetAttribute("override_font", 1);
            }

            connectionsElement->InsertEndChild(connElement);
        }
    }
    mapElement->InsertEndChild(connectionsElement);

    // Save Nodes
    auto rootElement = nodeToXml(map->root.get(), &doc);
    mapElement->InsertEndChild(rootElement);

    doc.SaveFile(filename.c_str());
}

std::shared_ptr<MindMap> MapSerializer::load(const std::string& filename) {
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
    if (rootName == TAG_MAP) {
        // New format
        // Load Theme
        map->theme.load(rootElement);

        // Load Node Tree
        tinyxml2::XMLElement* nodeElement = rootElement->FirstChildElement(TAG_NODE);
        if (nodeElement) {
            map->root = xmlToNode(nodeElement);
        }

        // Load Connections
        tinyxml2::XMLElement* connectionsElement = rootElement->FirstChildElement(TAG_CONNECTIONS);
        if (connectionsElement) {
            // First, assign sequential IDs to all nodes to enable connection restoration
            int currentId = 0;
            assignSequentialIds(map->root, currentId);

            for (tinyxml2::XMLElement* connElement = connectionsElement->FirstChildElement(TAG_CONNECTION);
                 connElement;
                 connElement = connElement->NextSiblingElement(TAG_CONNECTION)) {

                int fromId = connElement->IntAttribute(ATTR_FROM, -1);
                int toId = connElement->IntAttribute(ATTR_TO, -1);

                if (fromId != -1 && toId != -1) {
                    auto fromNode = findNodeById(map->root, fromId);
                    auto toNode = findNodeById(map->root, toId);

                    if (fromNode && toNode) {
                        auto conn = std::make_shared<Connection>(fromNode, toNode);

                        // Load connection properties
                        const char* text = connElement->Attribute(ATTR_TEXT);
                        if (text) conn->text = text;

                        const char* image = connElement->Attribute(ATTR_IMG);
                        if (image) conn->imagePath = image;

                        int r = connElement->IntAttribute(ATTR_R, 0);
                        int g = connElement->IntAttribute(ATTR_G, 0);
                        int b = connElement->IntAttribute(ATTR_B, 0);
                        conn->color = {r/255.0, g/255.0, b/255.0};

                        const char* font = connElement->Attribute(ATTR_FONT);
                        if (font) {
                            conn->fontDesc = font;
                            conn->overrideFont = connElement->IntAttribute("override_font", 0) == 1;
                        }

                        map->connections.push_back(conn);
                    }
                }
            }
        }
    } else if (rootName == TAG_NODE) {
        // Old format (Root is the node itself)
        map->root = xmlToNode(rootElement);
        // Theme remains default
    } else {
            throw std::runtime_error(_("Unknown file format"));
    }

    return map;
}

tinyxml2::XMLElement* MapSerializer::nodeToXml(const Node* node, tinyxml2::XMLDocument* doc) {
    auto element = doc->NewElement(TAG_NODE);

    element->SetAttribute(ATTR_TEXT, node->text.c_str());

    // Only save font attribute if it's being overridden
    if (node->overrideFont) {
        element->SetAttribute(ATTR_FONT, node->fontDesc.c_str());
    }

    // Only save image attributes if they exist
    if (!node->imagePath.empty()) {
        element->SetAttribute(ATTR_IMG, node->imagePath.c_str());
    }
    if (node->imgWidth > 0) {
        element->SetAttribute(ATTR_IMG_WIDTH, node->imgWidth);
    }
    if (node->imgHeight > 0) {
        element->SetAttribute(ATTR_IMG_HEIGHT, node->imgHeight);
    }

    element->SetAttribute(ATTR_CONN_TEXT, node->connText.c_str());

    // Only save connection font attribute if it's being overridden
    if (node->overrideConnFont) {
        element->SetAttribute(ATTR_CONN_FONT, node->connFontDesc.c_str());
    }

    // Only save connection image attribute if it exists
    if (!node->connImagePath.empty()) {
        element->SetAttribute(ATTR_CONN_IMG, node->connImagePath.c_str());
    }

    element->SetAttribute(ATTR_R, (int)(node->color.r*255));
    element->SetAttribute(ATTR_G, (int)(node->color.g*255));
    element->SetAttribute(ATTR_B, (int)(node->color.b*255));
    element->SetAttribute(ATTR_TEXT_R, (int)(node->textColor.r*255));
    element->SetAttribute(ATTR_TEXT_G, (int)(node->textColor.g*255));
    element->SetAttribute(ATTR_TEXT_B, (int)(node->textColor.b*255));
    element->SetAttribute(ATTR_X, node->x);
    element->SetAttribute(ATTR_Y, node->y);
    element->SetAttribute(ATTR_MANUAL, node->manualPosition ? 1 : 0);

    // Save override flags
    element->SetAttribute(ATTR_OVR_COLOR, node->overrideColor ? 1 : 0);
    element->SetAttribute(ATTR_OVR_TEXT_COLOR, node->overrideTextColor ? 1 : 0);
    element->SetAttribute(ATTR_OVR_FONT, node->overrideFont ? 1 : 0);
    element->SetAttribute(ATTR_OVR_CONN_FONT, node->overrideConnFont ? 1 : 0);

    // Add child nodes recursively
    for(const auto& child : node->children) {
        element->InsertEndChild(nodeToXml(child.get(), doc));
    }

    return element;
}

std::shared_ptr<Node> MapSerializer::xmlToNode(tinyxml2::XMLElement* element) {
    return xmlToNodeRecursive(element, 0);
}

std::shared_ptr<Node> MapSerializer::xmlToNodeRecursive(tinyxml2::XMLElement* element, int depth) {
    if (!element) return nullptr;
    
    const int MAX_RECURSION_DEPTH = 2000;
    if (depth > MAX_RECURSION_DEPTH) {
        throw std::runtime_error("Map too deep (recursion limit exceeded)");
    }

    // Get attributes from the XML element
    const char* text = element->Attribute(ATTR_TEXT);
    const char* font = element->Attribute(ATTR_FONT);
    const char* conn_font = element->Attribute(ATTR_CONN_FONT);
    const char* img = element->Attribute(ATTR_IMG);
    int iw = element->IntAttribute(ATTR_IMG_WIDTH, 0);
    int ih = element->IntAttribute(ATTR_IMG_HEIGHT, 0);
    const char* ctext = element->Attribute(ATTR_CONN_TEXT);
    const char* cimg = element->Attribute(ATTR_CONN_IMG);
    int r = element->IntAttribute(ATTR_R, 0);
    int g = element->IntAttribute(ATTR_G, 0);
    int b = element->IntAttribute(ATTR_B, 0);
    int tr = element->IntAttribute(ATTR_TEXT_R, 0);
    int tg = element->IntAttribute(ATTR_TEXT_G, 0);
    int tb = element->IntAttribute(ATTR_TEXT_B, 0);
    double x = element->DoubleAttribute(ATTR_X, 0.0);
    double y = element->DoubleAttribute(ATTR_Y, 0.0);
    bool manual = element->IntAttribute(ATTR_MANUAL, 0) == 1;

    // Load override flags with legacy compatibility
    bool ovr_c = false;
    if (element->QueryBoolAttribute(ATTR_OVR_COLOR, &ovr_c) != tinyxml2::XML_SUCCESS) {
        // Legacy: If ovr_c is missing, we consider it an override ONLY if the color attributes exist.
        if (element->Attribute(ATTR_R)) ovr_c = true;
    }

    bool ovr_t = false;
    if (element->QueryBoolAttribute(ATTR_OVR_TEXT_COLOR, &ovr_t) != tinyxml2::XML_SUCCESS) {
        // Legacy: Default to false (use theme) if not specified
        ovr_t = false;
    }

    bool ovr_f = false;
    if (element->QueryBoolAttribute(ATTR_OVR_FONT, &ovr_f) != tinyxml2::XML_SUCCESS) {
        // Legacy: If ovr_f is missing, we consider it an override ONLY if the font attribute exists.
        if (element->Attribute(ATTR_FONT)) ovr_f = true;
    }

    bool ovr_cf = false;
    if (element->QueryBoolAttribute(ATTR_OVR_CONN_FONT, &ovr_cf) != tinyxml2::XML_SUCCESS) {
        // Legacy: If ovr_cf is missing, we consider it an override ONLY if the conn_font attribute exists.
        if (element->Attribute(ATTR_CONN_FONT)) ovr_cf = true;
    }

    // Create node with the extracted data
    std::string textStr = text ? text : "";

    // For font, set the font string appropriately
    // Default font for new nodes is "Sans Bold 14" if not specified
    std::string fontStr = font ? font : "Sans Bold 14";

    std::string imgStr = img ? img : "";
    std::string ctextStr = ctext ? ctext : "";
    std::string connFontStr = conn_font ? conn_font : "";
    std::string cimgStr = cimg ? cimg : "";

    auto node = std::make_shared<Node>(textStr, E4Color{r/255.0, g/255.0, b/255.0});
    node->textColor = {tr/255.0, tg/255.0, tb/255.0};
    node->fontDesc = fontStr;
    node->imagePath = imgStr;
    node->imgWidth = iw;
    node->imgHeight = ih;
    node->connText = ctextStr;
    node->connImagePath = cimgStr;
    node->connFontDesc = connFontStr;
    node->x = x;
    node->y = y;
    node->manualPosition = manual;

    node->overrideColor = ovr_c;
    node->overrideTextColor = ovr_t;
    node->overrideFont = ovr_f;
    node->overrideConnFont = ovr_cf;

    // Process child elements
    for (tinyxml2::XMLElement* childElement = element->FirstChildElement(TAG_NODE);
         childElement;
         childElement = childElement->NextSiblingElement(TAG_NODE)) {
        auto childNode = xmlToNodeRecursive(childElement, depth + 1);
        if (childNode) {
            node->addChild(childNode);
        }
    }

    return node;
}