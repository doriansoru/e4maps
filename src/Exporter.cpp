#include "Exporter.hpp"
#include "LayoutAlgorithm.hpp"
#include "MindMapUtils.hpp"
#include "Constants.hpp"
#include <iostream>
#include <chrono>
#include <filesystem>

Exporter::Exporter(int w, int h) : width(w), height(h) {}

void Exporter::exportToPng(std::shared_ptr<MindMap> map, const std::string& filename, double dpi) {
    if (!map || !map->root) return;

    // 1. Ensure dimensions are pre-calculated and layout is applied consistently
    auto surface_temp = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1, 1);
    auto cr_temp = Cairo::Context::create(surface_temp);
    drawer.preCalculateNodeDimensions(map->root, map->theme, cr_temp);

    // If no manual positions, apply the same sequential layout used in GUI
    if (!hasManualPositionsRecursive(map->root)) {
        LayoutAlgorithms::calculateImprovedRadialLayout(map->root, 0, 0, 0, 2*M_PI, 0);
        LayoutAlgorithms::calculateForceDirectedLayout(map->root, 4096, 4096);
    }

    // 2. Calculate content bounds to determine canvas size
    double minX, minY, maxX, maxY;
    if (MindMapUtils::calculateMapBounds(map->root, minX, minY, maxX, maxY)) {
        // Add margin around the content
        double margin = E4Maps::EXPORT_MARGIN;
        minX -= margin;
        minY -= margin;
        maxX += margin;
        maxY += margin;

        // Calculate width and height of the content
        double contentWidth = maxX - minX;
        double contentHeight = maxY - minY;

        // Calculate scale factor based on DPI
        double scale = dpi / 72.0;

        // Create new surface with scaled dimensions
        int scaledWidth = static_cast<int>(contentWidth * scale);
        int scaledHeight = static_cast<int>(contentHeight * scale);

        // Safety check: Prevent huge allocations
        if (scaledWidth > 10000 || scaledHeight > 10000) {
                std::cerr << "Error: Export dimensions too large (" << scaledWidth << "x" << scaledHeight << "). Limit is 10000x10000." << std::endl;
                return;
        }

        auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, scaledWidth, scaledHeight);
        auto cr = Cairo::Context::create(surface);

        cr->scale(scale, scale);
        cr->translate(-minX, -minY);

        render(cr, map);

        surface->write_to_png(filename);
        std::cout << "Exported PNG: " << filename << " (" << scaledWidth << "x" << scaledHeight << ") at " << dpi << " DPI" << std::endl;
    }
}

void Exporter::exportToPdf(std::shared_ptr<MindMap> map, const std::string& filename) {
    if (!map || !map->root) return;

    // 1. Ensure dimensions are pre-calculated and layout is applied consistently
    auto surface_temp = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1, 1);
    auto cr_temp = Cairo::Context::create(surface_temp);
    drawer.preCalculateNodeDimensions(map->root, map->theme, cr_temp);

    if (!hasManualPositionsRecursive(map->root)) {
        LayoutAlgorithms::calculateImprovedRadialLayout(map->root, 0, 0, 0, 2*M_PI, 0);
        LayoutAlgorithms::calculateForceDirectedLayout(map->root, 4096, 4096);
    }

    // 2. Calculate content bounds
    double minX, minY, maxX, maxY;
    if (MindMapUtils::calculateMapBounds(map->root, minX, minY, maxX, maxY)) {
        double margin = E4Maps::EXPORT_MARGIN;
        minX -= margin;
        minY -= margin;
        maxX += margin;
        maxY += margin;

        double contentWidth = maxX - minX;
        double contentHeight = maxY - minY;

        auto surface = Cairo::PdfSurface::create(filename, contentWidth, contentHeight);
        auto cr = Cairo::Context::create(surface);

        cr->translate(-minX, -minY);
        render(cr, map);

        std::cout << "Exported PDF: " << filename << " (" << contentWidth << "x" << contentHeight << ")" << std::endl;
    }
}

void Exporter::exportToFreeplane(std::shared_ptr<MindMap> map, const std::string& filename) {
    tinyxml2::XMLDocument doc;

    // Create the root map element
    auto mapElement = doc.NewElement("map");
    mapElement->SetAttribute("version", "1.3.0");
    doc.InsertFirstChild(mapElement);

    if (map && map->root) {
        // Generate base timestamp for the root node
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        exportNodeToFreeplaneImproved(doc, mapElement, map->root, generateId(), now);
    }

    doc.SaveFile(filename.c_str());
}

void Exporter::render(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<MindMap> map) {
    cr->set_source_rgb(1, 1, 1);
    cr->paint();
    if (!map || !map->root) return;

    // Dimensions are already calculated in exportToPng/Pdf
    drawer.drawNode(cr, map->root, 0, map->theme, nullptr, {}, map->connections);
}

bool Exporter::hasManualPositionsRecursive(std::shared_ptr<Node> node) {
    if (!node) return false;
    if (node->manualPosition) return true;
    for (auto& child : node->children) {
        if (hasManualPositionsRecursive(child)) {
            return true;
        }
    }
    return false;
}

int Exporter::countNodesInTree(std::shared_ptr<Node> node) {
    if (!node) return 0;
    int count = 1;
    for (auto& child : node->children) {
        count += countNodesInTree(child);
    }
    return count;
}

std::string Exporter::generateId() {
    static int idCounter = 0;
    return "ID_" + std::to_string(idCounter++);
}

void Exporter::exportNodeToFreeplaneImproved(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElement, std::shared_ptr<Node> node, const std::string& nodeId, long baseTimestamp) {
    // Calculate timestamp for this node
    long nodeTimestamp = baseTimestamp;

    // Create node element with required attributes
    auto nodeElement = doc.NewElement("node");
    nodeElement->SetAttribute("TEXT", node->text.c_str());
    nodeElement->SetAttribute("ID", nodeId.c_str());
    nodeElement->SetAttribute("CREATED", static_cast<int64_t>(nodeTimestamp));
    nodeElement->SetAttribute("MODIFIED", static_cast<int64_t>(nodeTimestamp));

    if (node->manualPosition) {
        if (node->x < 0) {
            nodeElement->SetAttribute("POSITION", "left");
        } else {
            nodeElement->SetAttribute("POSITION", "right");
        }
    }

    if (node->color.r != 0.0 || node->color.g != 0.0 || node->color.b != 0.0) {
        int r = static_cast<int>(node->color.r * 255);
        int g = static_cast<int>(node->color.g * 255);
        int b = static_cast<int>(node->color.b * 255);
        char colorStr[8];
        snprintf(colorStr, sizeof(colorStr), "#%02X%02X%02X", r, g, b);
        nodeElement->SetAttribute("COLOR", colorStr);
    }

    if (!node->fontDesc.empty()) {
        auto fontElement = doc.NewElement("font");
        fontElement->SetAttribute("NAME", node->fontDesc.c_str());
        if (node->fontDesc.find("Bold") != std::string::npos) {
            fontElement->SetAttribute("BOLD", "true");
        }
        nodeElement->InsertEndChild(fontElement);
    }

    if (!node->imagePath.empty()) {
        auto richcontentElement = doc.NewElement("richcontent");
        richcontentElement->SetAttribute("TYPE", "DETAILS");
        auto htmlElement = doc.NewElement("html");
        auto bodyElement = doc.NewElement("body");
        auto imgElement = doc.NewElement("img");
        imgElement->SetAttribute("src", node->imagePath.c_str());
        bodyElement->InsertEndChild(imgElement);
        htmlElement->InsertEndChild(bodyElement);
        richcontentElement->InsertEndChild(htmlElement);
        nodeElement->InsertEndChild(richcontentElement);
    }

    for (auto& child : node->children) {
        exportNodeToFreeplaneImproved(doc, nodeElement, child, generateId(), nodeTimestamp + 1000);
    }
    parentElement->InsertEndChild(nodeElement);
}
