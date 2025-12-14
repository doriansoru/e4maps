#ifndef EXPORTER_HPP
#define EXPORTER_HPP

#include "MindMap.hpp"
#include "MindMapDrawer.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "LayoutAlgorithm.hpp" // Include for layout algorithms
#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "MindMapUtils.hpp"
#include "tinyxml2.h"

class Exporter {
    int width;
    int height;
    MindMapDrawer drawer; // Instance of MindMapDrawer
    const double PI = 3.14159265359;

public:
    Exporter(int w, int h) : width(w), height(h) {}

    void exportToPng(std::shared_ptr<MindMap> map, const std::string& filename, double dpi = 72.0) {
        // Calculate content bounds to determine canvas size
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

            // Calculate scale factor based on DPI (default is 72 DPI, so scale = 1.0)
            // For 300 DPI: scale = 300/72 = 4.167, for 600 DPI: scale = 600/72 = 8.333
            double scale = dpi / 72.0;

            // Create new surface with scaled dimensions
            int scaledWidth = static_cast<int>(contentWidth * scale);
            int scaledHeight = static_cast<int>(contentHeight * scale);

            auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, scaledWidth, scaledHeight);
            auto cr = Cairo::Context::create(surface);

            // Scale the context to achieve higher DPI while keeping the same visual proportions
            cr->scale(scale, scale);

            // Translate coordinates to compensate for the offset
            cr->translate(-minX, -minY);

            // Render the content
            render(cr, map);

            surface->write_to_png(filename);
            std::cout << "Exported PNG: " << filename << " (" << scaledWidth << "x" << scaledHeight << ") at " << dpi << " DPI" << std::endl;
        } else {
            // Fallback: export empty canvas if no content exists
            double scale = dpi / 72.0;
            int scaledWidth = static_cast<int>(800 * scale);
            int scaledHeight = static_cast<int>(600 * scale);

            auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, scaledWidth, scaledHeight);
            auto cr = Cairo::Context::create(surface);

            // Scale the context to maintain DPI consistency
            cr->scale(scale, scale);

            cr->set_source_rgb(1, 1, 1);
            cr->paint();
            surface->write_to_png(filename);
            std::cout << "Exported PNG: " << filename << " (empty) at " << dpi << " DPI" << std::endl;
        }
    }

    void exportToPdf(std::shared_ptr<MindMap> map, const std::string& filename) {
        // Calculate content bounds to determine canvas size
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

            // Create PDF surface with exact content size (in points)
            auto surface = Cairo::PdfSurface::create(filename, contentWidth, contentHeight);
            auto cr = Cairo::Context::create(surface);

            // Translate coordinates to compensate for the offset
            cr->translate(-minX, -minY);

            // Render the content
            render(cr, map);

            std::cout << "Exported PDF: " << filename << " (" << contentWidth << "x" << contentHeight << ")" << std::endl;
        } else {
            // Fallback: export empty canvas if no content exists
            auto surface = Cairo::PdfSurface::create(filename, 800, 600);
            auto cr = Cairo::Context::create(surface);
            cr->set_source_rgb(1, 1, 1);
            cr->paint();
            std::cout << "Exported PDF: " << filename << " (empty)" << std::endl;
        }
    }

    void exportToFreeplane(std::shared_ptr<MindMap> map, const std::string& filename) {
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



    void render(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<MindMap> map) {
        cr->set_source_rgb(1, 1, 1);
        cr->paint();
        if (!map || !map->root) return;

        // Pre-calculate all node dimensions to ensure arrows are positioned correctly
        drawer.preCalculateNodeDimensions(map->root, map->theme, cr);

        // Check if any nodes have manual positioning
        bool hasManualPositions = hasManualPositionsRecursive(map->root);

        if (hasManualPositions) {
            // If nodes have been manually positioned, respect their positions
            // No layout algorithm should be applied
        } else {
            // Apply improved layout for better readability during export if no manual positions
            // Use force-directed layout for complex maps for better readability in export
            int nodeCount = countNodesInTree(map->root);
            if (nodeCount > 20) {
                // Use the force-directed algorithm for export with a large canvas
                LayoutAlgorithms::calculateForceDirectedLayout(map->root, 4096, 4096);
            } else {
                // Apply improved radial layout for simpler maps
                calculateImprovedRadialLayoutForExport(map->root);
            }
        }

        drawer.drawNode(cr, map->root, 0, map->theme);
    }

    // Helper to check if any nodes have manual positioning
    bool hasManualPositionsRecursive(std::shared_ptr<Node> node) {
        if (!node) return false;
        if (node->manualPosition) return true;
        for (auto& child : node->children) {
            if (hasManualPositionsRecursive(child)) {
                return true;
            }
        }
        return false;
    }

    // Helper to count nodes in tree for complexity analysis
    int countNodesInTree(std::shared_ptr<Node> node) {
        if (!node) return 0;
        int count = 1;
        for (auto& child : node->children) {
            count += countNodesInTree(child);
        }
        return count;
    }

    // Calculate improved radial layout specifically for export
    void calculateImprovedRadialLayoutForExport(std::shared_ptr<Node> node) {
        if (!node) return;

        // Start with the root at center if it doesn't have manual position
        if (node->isRoot() && !node->manualPosition) {
            node->x = 0;
            node->y = 0;
        }

        // Use the improved algorithm for children, but respect manual positions
        if (!node->children.empty()) {
            LayoutAlgorithms::calculateImprovedRadialLayout(node, node->x, node->y, 0, 2*M_PI, 0);
        }

        // Recursively apply to all children
        for (auto& child : node->children) {
            calculateImprovedRadialLayoutForExport(child);
        }
    }
    
    // Helper methods for Freeplane export

    // Helper methods for Freeplane export
    std::string generateId() {
        static int idCounter = 0;
        return "ID_" + std::to_string(idCounter++);
    }

    void exportNodeToFreeplaneImproved(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parentElement, std::shared_ptr<Node> node, const std::string& nodeId, long baseTimestamp) {
        // Calculate timestamp for this node - each node gets a slightly different timestamp
        long nodeTimestamp = baseTimestamp; // Add time based on level

        // Create node element with required attributes
        auto nodeElement = doc.NewElement("node");
        nodeElement->SetAttribute("TEXT", node->text.c_str());
        nodeElement->SetAttribute("ID", nodeId.c_str());
        nodeElement->SetAttribute("CREATED", static_cast<int64_t>(nodeTimestamp));
        nodeElement->SetAttribute("MODIFIED", static_cast<int64_t>(nodeTimestamp));

        // Add position information if available
        if (node->manualPosition) {
            // Determine position based on relative coordinates (simplified approach)
            if (node->x < 0) {
                nodeElement->SetAttribute("POSITION", "left");
            } else {
                nodeElement->SetAttribute("POSITION", "right");
            }
        }

        // Add color information (if different from default)
        if (node->color.r != 0.0 || node->color.g != 0.0 || node->color.b != 0.0) {
            int r = static_cast<int>(node->color.r * 255);
            int g = static_cast<int>(node->color.g * 255);
            int b = static_cast<int>(node->color.b * 255);
            char colorStr[8];
            snprintf(colorStr, sizeof(colorStr), "#%02X%02X%02X", r, g, b);
            nodeElement->SetAttribute("COLOR", colorStr);
        }

        // Add style information based on font properties
        std::string fontStr = node->fontDesc;
        if (fontStr.find("Bold") != std::string::npos) {
            nodeElement->SetAttribute("STYLE", "bubble");
        }

        // Add font information if needed
        if (!node->fontDesc.empty()) {
            auto fontElement = doc.NewElement("font");
            fontElement->SetAttribute("NAME", node->fontDesc.c_str());
            // Extract font size and other properties from font description
            if (node->fontDesc.find("Bold") != std::string::npos) {
                fontElement->SetAttribute("BOLD", "true");
            }
            nodeElement->InsertEndChild(fontElement);
        }

        // Add image as richcontent if present (separate from text)
        if (!node->imagePath.empty()) {
            auto richcontentElement = doc.NewElement("richcontent");
            richcontentElement->SetAttribute("TYPE", "DETAILS");

            // Create HTML structure for the image
            auto htmlElement = doc.NewElement("html");
            auto headElement = doc.NewElement("head");
            auto bodyElement = doc.NewElement("body");
            auto pElement = doc.NewElement("p");
            auto imgElement = doc.NewElement("img");
            imgElement->SetAttribute("src", node->imagePath.c_str());

            // If image dimensions are available, use them; otherwise use reasonable defaults
            if (node->imgWidth > 0 && node->imgHeight > 0) {
                imgElement->SetAttribute("width", node->imgWidth);
                imgElement->SetAttribute("height", node->imgHeight);
            } else {
                // Use reasonable default sizes for Freeplane display
                imgElement->SetAttribute("width", 100);
                imgElement->SetAttribute("height", 100);
            }

            // Build the HTML structure
            pElement->InsertEndChild(imgElement);
            bodyElement->InsertEndChild(pElement);
            htmlElement->InsertEndChild(headElement);
            htmlElement->InsertEndChild(bodyElement);
            richcontentElement->InsertEndChild(htmlElement);
            nodeElement->InsertEndChild(richcontentElement);
        }

        // Process all children
        for (auto& child : node->children) {
            exportNodeToFreeplaneImproved(doc, nodeElement, child, generateId(), nodeTimestamp + 1000);
        }

        // Add the completed node element to the parent
        parentElement->InsertEndChild(nodeElement);
    }
};

#endif // RENDERER_HPP
