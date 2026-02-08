#include "MindMapDrawer.hpp"
#include "MindMapUtils.hpp"
#include "ImageCache.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "NodeRenderer.hpp"
#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void MindMapDrawer::preCalculateNodeDimensions(std::shared_ptr<Node> node, const Theme& theme, const Cairo::RefPtr<Cairo::Context>& cr, int depth) {
    if (!node) return;
    calculateNodeDimensions(node, theme, cr, depth);
    for (auto& child : node->children) {
        preCalculateNodeDimensions(child, theme, cr, depth + 1);
    }
}

void MindMapDrawer::calculateNodeDimensions(std::shared_ptr<Node> node, const Theme& theme, const Cairo::RefPtr<Cairo::Context>& cr, int depth) {
    if (!node) return;

    NodeStyle style = theme.getStyle(depth);
    if (node->overrideFont && !node->fontDesc.empty()) {
        style.fontDescription = Pango::FontDescription(node->fontDesc);
    }

    Glib::RefPtr<Pango::Layout> layout;
    std::shared_ptr<CachedLayoutData> cache;
    if (node->_layoutCache) {
        cache = std::static_pointer_cast<CachedLayoutData>(node->_layoutCache);
    }

    std::string currentFontDesc = style.fontDescription.to_string();

    if (cache && cache->layout && cache->text == node->text && cache->fontDesc == currentFontDesc) {
        layout = cache->layout;
    } else {
        layout = Pango::Layout::create(cr);
        Utils::setPangoLayoutText(layout, node->text);
        layout->set_font_description(style.fontDescription);
        
        // Step 1: Measure natural width without constraints
        layout->set_width(-1); 
        Pango::Rectangle ink, logical;
        layout->get_extents(ink, logical);
        double naturalWidth = logical.get_width() / (double)Pango::SCALE;

        // Step 2: Apply wrapping only if needed
        if (naturalWidth > E4Maps::MAX_NODE_WIDTH) {
            layout->set_width(E4Maps::MAX_NODE_WIDTH * Pango::SCALE);
            layout->set_wrap(Pango::WRAP_WORD);
        }
        
        layout->set_alignment(Pango::ALIGN_CENTER);
        
        auto newCache = std::make_shared<CachedLayoutData>();
        newCache->layout = layout;
        newCache->text = node->text;
        newCache->fontDesc = currentFontDesc;
        node->_layoutCache = newCache;
        cache = newCache;
    }
    
    // Final measurement of the (possibly wrapped) layout
    Pango::Rectangle ink, logical;
    layout->get_extents(ink, logical);
    double textW = std::ceil(logical.get_width() / (double)Pango::SCALE);
    double textH = std::ceil(logical.get_height() / (double)Pango::SCALE);

    // Sync layout width to its measured width to avoid centering offset issues in Renderer
    layout->set_width(logical.get_width());

    cache->width = textW;
    cache->height = textH;

    double contentWidth = textW;
    double contentHeight = textH;
    double imgW = 0, imgH = 0;

    auto pb = getCachedImage(node->imagePath, node->imgWidth, node->imgHeight);
    if (pb) {
        imgW = pb->get_width(); imgH = pb->get_height();
        contentWidth = std::max(contentWidth, imgW);
        contentHeight += imgH + 5;
    }

    // Standard padding + 20px breathing room (10px per side)
    node->width = contentWidth + style.horizontalPadding * 2 + 20.0;
    node->height = contentHeight + style.verticalPadding * 2;
}

Glib::RefPtr<Gdk::Pixbuf> MindMapDrawer::getCachedImage(const std::string& path, int reqW, int reqH) {
    return ImageCache::getInstance().getCachedImage(path, reqW, reqH);
}

void MindMapDrawer::clearImageCache() {
    ImageCache::getInstance().clear();
}

void MindMapDrawer::drawMap(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> root, const Theme& theme, std::shared_ptr<Node> selectedNode, const std::vector<std::shared_ptr<Node>>& selectedNodes, const std::vector<std::shared_ptr<Connection>>& connections, std::shared_ptr<Connection> selectedConnection, std::shared_ptr<Connection> hoveredConnection) {
    if (!root) return;
    drawConnectionsRecursive(cr, root, 0, theme, connections, selectedConnection, hoveredConnection);
    drawNodesRecursive(cr, root, 0, theme, selectedNode, selectedNodes);
}

void MindMapDrawer::drawConnectionsRecursive(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, int depth, const Theme& theme, const std::vector<std::shared_ptr<Connection>>& connections, std::shared_ptr<Connection> selectedConnection, std::shared_ptr<Connection> hoveredConnection) {
    if (!node) return;
    NodeStyle style = theme.getStyle(depth);

    for (auto& child : node->children) {
        cr->save();
        Cairo::RefPtr<Cairo::Pattern> connColor = style.connectionColor;
        if (child->overrideColor) {
            connColor = Cairo::SolidPattern::create_rgb(child->color.r, child->color.g, child->color.b);
        }
        
        E4Color arrowColor = child->color;
        auto solidPattern = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(connColor);
        if (solidPattern) {
            double r, g, b, a;
            solidPattern->get_rgba(r, g, b, a);
            arrowColor = {r, g, b};
        }

        NodeRenderer::drawConnection(cr, node, child, theme, depth, connColor, arrowColor, false, false);

        if (!child->connText.empty() || !child->connImagePath.empty()) {
            double mx, my, tangent_angle;
            double dx = child->x - node->x;
            double dy = child->y - node->y;
            double distance = std::sqrt(dx*dx + dy*dy);

            if (style.connectionType == 1) { // Organic
                double ctrlX, ctrlY;
                MindMapUtils::calculateOrganicBezierControlPoint(node->x, node->y, child->x, child->y, depth, ctrlX, ctrlY);
                double t = 0.5;
                mx = (1-t)*(1-t)*node->x + 2*(1-t)*t*ctrlX + t*t*child->x;
                my = (1-t)*(1-t)*node->y + 2*(1-t)*t*ctrlY + t*t*child->y;
                tangent_angle = std::atan2(2*(1-t)*(ctrlY - node->y) + 2*t*(child->y - ctrlY),
                                           2*(1-t)*(ctrlX - node->x) + 2*t*(child->x - ctrlX));
            } else { // Traditional
                double t = 0.5;
                double cpDist = distance * 0.4;
                double geoAngle = std::atan2(dy, dx);
                double p1x = node->x + cpDist * std::cos(geoAngle);
                double p1y = node->y + cpDist * std::sin(geoAngle);
                double p2x = child->x - cpDist * std::cos(geoAngle);
                double p2y = child->y - cpDist * std::sin(geoAngle);
                mx = (1-t)*(1-t)*(1-t)*node->x + 3*(1-t)*(1-t)*t*p1x + 3*(1-t)*t*t*p2x + t*t*t*child->x;
                my = (1-t)*(1-t)*(1-t)*node->y + 3*(1-t)*(1-t)*t*p1y + 3*(1-t)*t*t*p2y + t*t*t*child->y;
                tangent_angle = std::atan2(3*(1-t)*(1-t)*(p1y-node->y) + 6*(1-t)*t*(p2y-p1y) + 3*t*t*(child->y-p2y),
                                           3*(1-t)*(1-t)*(p1x-node->x) + 6*(1-t)*t*(p2x-p1x) + 3*t*t*(child->x-p2x));
            }

            cr->save();
            cr->translate(mx, my);
            cr->rotate(tangent_angle);
            if (std::abs(tangent_angle) > M_PI/2) cr->rotate(M_PI);

            int tw = 0, th = 0;
            double contentW = 0;
            if (!child->connImagePath.empty()) {
                auto pb = getCachedImage(child->connImagePath, 24, 24);
                if (pb) contentW += pb->get_width();
            }
            if (!child->connText.empty()) {
                auto layout = Pango::Layout::create(cr);
                Utils::setPangoLayoutText(layout, child->connText);
                layout->set_font_description(child->overrideConnFont ? Pango::FontDescription(child->connFontDesc) : style.connectionFontDescription);
                layout->get_pixel_size(tw, th);
                contentW += tw;
            }

            double curX = -contentW / 2.0;
            if (!child->connImagePath.empty()) {
                auto pb = getCachedImage(child->connImagePath, 24, 24);
                if (pb) {
                    Gdk::Cairo::set_source_pixbuf(cr, pb, curX, -pb->get_height() - 2);
                    cr->paint();
                    curX += pb->get_width();
                }
            }
            if (!child->connText.empty()) {
                cr->set_source_rgba(1, 1, 1, 0.8);
                NodeRenderer::roundedRectanglePath(cr, curX - 2, -th - 4, tw + 4, th + 4, 3.0);
                cr->fill();
                cr->set_source_rgb(0.3, 0.3, 0.3);
                auto layout = Pango::Layout::create(cr);
                Utils::setPangoLayoutText(layout, child->connText);
                layout->set_font_description(child->overrideConnFont ? Pango::FontDescription(child->connFontDesc) : style.connectionFontDescription);
                cr->move_to(curX, -th - 2);
                layout->show_in_cairo_context(cr);
            }
            cr->restore();
        }
        cr->restore();
        drawConnectionsRecursive(cr, child, depth + 1, theme, connections, selectedConnection, hoveredConnection);
    }
    drawArbitraryConnectionsForNode(cr, node, connections, theme, depth, selectedConnection, hoveredConnection);
}

void MindMapDrawer::drawNodesRecursive(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, int depth, const Theme& theme, std::shared_ptr<Node> selectedNode, const std::vector<std::shared_ptr<Node>>& selectedNodes) {
    if (!node) return;
    double clipX1, clipY1, clipX2, clipY2;
    cr->get_clip_extents(clipX1, clipY1, clipX2, clipY2);
    if (node->x + node->width/2 + 20 >= clipX1 && node->x - node->width/2 - 20 <= clipX2 &&
        node->y + node->height/2 + 20 >= clipY1 && node->y - node->height/2 - 20 <= clipY2) {
        bool isSel = (node == selectedNode) || (std::find(selectedNodes.begin(), selectedNodes.end(), node) != selectedNodes.end());
        NodeRenderer::drawNodeContent(cr, node, theme, depth, isSel, false);
    }
    for (auto& child : node->children) {
        drawNodesRecursive(cr, child, depth + 1, theme, selectedNode, selectedNodes);
    }
}

void MindMapDrawer::drawNode(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, int depth, const Theme& theme, std::shared_ptr<Node> selectedNode, const std::vector<std::shared_ptr<Node>>& selectedNodes, const std::vector<std::shared_ptr<Connection>>& connections, std::shared_ptr<Connection> selectedConnection, std::shared_ptr<Connection> hoveredConnection) {
    if (depth == 0) drawMap(cr, node, theme, selectedNode, selectedNodes, connections, selectedConnection, hoveredConnection);
}

void MindMapDrawer::drawArbitraryConnectionsForNode(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, const std::vector<std::shared_ptr<Connection>>& connections, const Theme& theme, int depth, std::shared_ptr<Connection> selectedConnection, std::shared_ptr<Connection> hoveredConnection) {
    for (const auto& conn : connections) {
        if (!conn || !conn->from || !conn->to || conn->from != node) continue;
        NodeStyle style = theme.getStyle(depth);
        Cairo::RefPtr<Cairo::Pattern> colorPattern = conn->overrideFont ? 
            Cairo::RefPtr<Cairo::Pattern>(Cairo::SolidPattern::create_rgb(conn->color.r, conn->color.g, conn->color.b)) : 
            style.connectionColor;
        NodeRenderer::drawConnection(cr, conn->from, conn->to, theme, depth, colorPattern, conn->color, (selectedConnection == conn), (hoveredConnection == conn));
        if (!conn->text.empty() || !conn->imagePath.empty()) {
            double ctrlX, ctrlY;
            MindMapUtils::calculateOrganicBezierControlPoint(conn->from->x, conn->from->y, conn->to->x, conn->to->y, depth, ctrlX, ctrlY);
            double t = 0.5;
            double mx = (1-t)*(1-t)*conn->from->x + 2*(1-t)*t*ctrlX + t*t*conn->to->x;
            double my = (1-t)*(1-t)*conn->from->y + 2*(1-t)*t*ctrlY + t*t*conn->to->y;
            double tangent_angle = std::atan2(2*(1-t)*(ctrlY - conn->from->y) + 2*t*(conn->to->y - ctrlY),
                                               2*(1-t)*(ctrlX - conn->from->x) + 2*t*(conn->to->x - ctrlX));
            cr->save();
            cr->translate(mx, my);
            cr->rotate(tangent_angle);
            if (std::abs(tangent_angle) > M_PI/2) cr->rotate(M_PI);
            int tw = 0, th = 0;
            double contentW = 0;
            if (!conn->imagePath.empty()) {
                auto pb = getCachedImage(conn->imagePath, 24, 24);
                if (pb) contentW += pb->get_width();
            }
            if (!conn->text.empty()) {
                auto layout = Pango::Layout::create(cr);
                Utils::setPangoLayoutText(layout, conn->text);
                layout->set_font_description(conn->overrideFont ? Pango::FontDescription(conn->fontDesc) : theme.getStyle(depth).connectionFontDescription);
                layout->get_pixel_size(tw, th);
                contentW += tw;
            }
            double curX = -contentW / 2.0;
            if (!conn->imagePath.empty()) {
                auto pb = getCachedImage(conn->imagePath, 24, 24);
                if (pb) {
                    Gdk::Cairo::set_source_pixbuf(cr, pb, curX, -pb->get_height() - 2);
                    cr->paint();
                    curX += pb->get_width();
                }
            }
            if (!conn->text.empty()) {
                cr->set_source_rgba(1, 1, 1, 0.8);
                NodeRenderer::roundedRectanglePath(cr, curX - 2, -th - 4, tw + 4, th + 4, 3.0);
                cr->fill();
                cr->set_source_rgb(0.3, 0.3, 0.3);
                auto layout = Pango::Layout::create(cr);
                Utils::setPangoLayoutText(layout, conn->text);
                layout->set_font_description(conn->overrideFont ? Pango::FontDescription(conn->fontDesc) : theme.getStyle(depth).connectionFontDescription);
                cr->move_to(curX, -th - 2);
                layout->show_in_cairo_context(cr);
            }
            cr->restore();
        }
    }
}
