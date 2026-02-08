#ifndef EXPORTER_HPP
#define EXPORTER_HPP

#include "MindMap.hpp"
#include "MindMapDrawer.hpp"
#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <string>
#include <memory>
#include "tinyxml2.h"

class Exporter {
    int width;
    int height;
    MindMapDrawer drawer; // Instance of MindMapDrawer
    const double PI = 3.14159265359;

public:
    Exporter(int w, int h);

    void exportToPng(std::shared_ptr<MindMap> map, const std::string& filename, double dpi = 72.0);

    void exportToPdf(std::shared_ptr<MindMap> map, const std::string& filename);

    void exportToFreeplane(std::shared_ptr<MindMap> map, const std::string& filename);

private:
    void render(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<MindMap> map);

    // Helper to check if any nodes have manual positioning
    bool hasManualPositionsRecursive(std::shared_ptr<Node> node);

    // Helper to count nodes in tree for complexity analysis
    int countNodesInTree(std::shared_ptr<Node> node);

    // Helper methods for Freeplane export
    std::string generateId();

    void exportNodeToFreeplaneImproved(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElement, std::shared_ptr<Node> node, const std::string& nodeId, long baseTimestamp);
};

#endif // EXPORTER_HPP