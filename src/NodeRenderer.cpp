#include "NodeRenderer.hpp"
#include "Utils.hpp"
#include "ImageCache.hpp"
#include "Constants.hpp"
#include "MindMapUtils.hpp"
#include <cmath>
#include <limits>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void NodeRenderer::roundedRectanglePath(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double width, double height, double radius) {
    double degrees = M_PI / 180.0;
    cr->begin_new_sub_path();
    cr->arc(x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cr->arc(x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cr->arc(x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cr->arc(x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cr->close_path();
}

double NodeRenderer::getDistanceToRectBoundary(double width, double height, double angle) {
    double cosA = std::cos(angle);
    double sinA = std::sin(angle);
    double halfW = width / 2.0;
    double halfH = height / 2.0;
    double distX = (std::abs(cosA) > 1e-6) ? std::abs(halfW / cosA) : std::numeric_limits<double>::max();
    double distY = (std::abs(sinA) > 1e-6) ? std::abs(halfH / sinA) : std::numeric_limits<double>::max();
    return std::min(distX, distY);
}

void NodeRenderer::drawArrowHead(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double angle, double size, E4Color color) {
    cr->save();
    cr->set_source_rgb(color.r, color.g, color.b);
    cr->set_line_width(2.0);
    cr->set_line_cap(Cairo::LINE_CAP_ROUND);
    cr->set_line_join(Cairo::LINE_JOIN_ROUND);
    cr->translate(x, y);
    cr->rotate(angle);
    double arrowHalfWidth = size * 0.8;
    cr->move_to(0, 0);
    cr->line_to(-size * 1.2, -arrowHalfWidth);
    cr->line_to(-size * 0.6, 0);
    cr->line_to(-size * 1.2, arrowHalfWidth);
    cr->close_path();
    cr->fill();
    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->set_line_width(1.0);
    cr->stroke();
    cr->restore();
}

void NodeRenderer::drawOrganicArrow(const Cairo::RefPtr<Cairo::Context>& cr,
                                   double startX, double startY,
                                   double endX, double endY,
                                   double nodeWidth, double nodeHeight,
                                   double width,
                                   const Cairo::RefPtr<Cairo::Pattern>& color,
                                   E4Color arrowColor,
                                   int depth) {
    cr->save();
    double dx = endX - startX;
    double dy = endY - startY;
    double distance = std::sqrt(dx * dx + dy * dy);
    if (distance < 0.1) {
        cr->restore();
        return;
    }
    double ctrlX, ctrlY;
    MindMapUtils::calculateOrganicBezierControlPoint(startX, startY, endX, endY, depth, ctrlX, ctrlY);
    double endTangentX = 3 * (endX - ctrlX);
    double endTangentY = 3 * (endY - ctrlY);
    double approachAngle = std::atan2(endTangentY, endTangentX);
    double exitAngle = approachAngle + M_PI;
    double distToBoundary = getDistanceToRectBoundary(nodeWidth, nodeHeight, exitAngle);
    double finalEndX = endX + std::cos(exitAngle) * distToBoundary;
    double finalEndY = endY + std::sin(exitAngle) * distToBoundary;
    cr->set_line_width(width * 2.0);
    cr->set_line_cap(Cairo::LINE_CAP_ROUND);
    cr->set_source(color);
    cr->move_to(startX, startY);
    cr->curve_to(ctrlX, ctrlY, ctrlX, ctrlY, finalEndX, finalEndY);
    cr->stroke();
    double tangentX = 3 * (finalEndX - ctrlX);
    double tangentY = 3 * (finalEndY - ctrlY);
    double arrowAngle = std::atan2(tangentY, tangentX);
    drawArrowHead(cr, finalEndX, finalEndY, arrowAngle, std::max(width * 3.0, 8.0), arrowColor);
    cr->restore();
}

void NodeRenderer::drawConnection(const Cairo::RefPtr<Cairo::Context>& cr, 
                                  std::shared_ptr<Node> from, 
                                  std::shared_ptr<Node> to, 
                                  const Theme& theme, 
                                  int depth,
                                  Cairo::RefPtr<Cairo::Pattern> colorPattern, 
                                  E4Color arrowColor,
                                  bool isSelected, 
                                  bool isHovered) {
    if (!from || !to) return;
    NodeStyle style = theme.getStyle(depth);
    double connectionWidth = style.connectionWidth;
    if (isSelected || isHovered) {
        cr->save();
        cr->set_source_rgba(0.2, 0.6, 1.0, 0.4);
        drawOrganicArrow(cr, from->x, from->y, to->x, to->y,
                        to->width, to->height, connectionWidth + 4.0, 
                        Cairo::SolidPattern::create_rgba(0.2, 0.6, 1.0, 0.4), 
                        arrowColor, depth);
        cr->restore();
    }
    if (style.connectionType == 1) { 
        drawOrganicArrow(cr, from->x, from->y, to->x, to->y,
                        to->width, to->height, connectionWidth, colorPattern, arrowColor, depth);
    } else { 
        cr->save();
        cr->set_source(colorPattern);
        cr->set_line_width(connectionWidth);
        cr->set_line_cap(Cairo::LINE_CAP_ROUND);
        double dx = to->x - from->x;
        double dy = to->y - from->y;
        double dist = std::sqrt(dx*dx + dy*dy);
        double cpDist = dist * 0.4;
        double geoAngle = std::atan2(dy, dx);
        double p1x = from->x + cpDist * std::cos(geoAngle);
        double p1y = from->y + cpDist * std::sin(geoAngle);
        double p2x = to->x - cpDist * std::cos(geoAngle);
        double p2y = to->y - cpDist * std::sin(geoAngle);
        cr->move_to(from->x, from->y);
        cr->curve_to(p1x, p1y, p2x, p2y, to->x, to->y);
        cr->stroke();
        double endTangentX = 3 * (to->x - p2x);
        double endTangentY = 3 * (to->y - p2y);
        double arrowAngle = std::atan2(endTangentY, endTangentX);
        double exitAngle = arrowAngle + M_PI;
        double distToBoundary = getDistanceToRectBoundary(to->width, to->height, exitAngle);
        drawArrowHead(cr, to->x + std::cos(exitAngle) * distToBoundary, 
                      to->y + std::sin(exitAngle) * distToBoundary, 
                      arrowAngle, std::max(10.0, 18.0 - depth * 1.2), arrowColor);
        cr->restore();
    }
}

void NodeRenderer::drawNodeContent(const Cairo::RefPtr<Cairo::Context>& cr, 
                                   std::shared_ptr<Node> node, 
                                   const Theme& theme, 
                                   int depth, 
                                   bool isSelected, 
                                   bool isEditing) {
    if (!node) return;
    NodeStyle style = theme.getStyle(depth);
    if (node->overrideColor) style.backgroundColor = Cairo::SolidPattern::create_rgb(node->color.r, node->color.g, node->color.b);
    if (node->overrideTextColor) style.textColor = Cairo::SolidPattern::create_rgb(node->textColor.r, node->textColor.g, node->textColor.b);
    
    double x = node->x;
    double y = node->y;
    double width = node->width;
    double height = node->height;

    if (style.shadowColor) {
        cr->save();
        roundedRectanglePath(cr, x - width/2 + style.shadowOffsetX, y - height/2 + style.shadowOffsetY, width, height, style.cornerRadius);
        cr->set_source(style.shadowColor);
        cr->fill();
        cr->restore();
    }

    cr->save();
    roundedRectanglePath(cr, x - width/2, y - height/2, width, height, style.cornerRadius);
    cr->set_source(isSelected ? style.backgroundHoverColor : style.backgroundColor);
    cr->fill_preserve();
    if (isSelected) {
        cr->set_source_rgb(0.2, 0.6, 1.0);
        cr->set_line_width(2.5);
    } else {
        cr->set_source(style.borderColor);
        cr->set_line_width(style.borderWidth);
    }
    cr->stroke();
    cr->restore();

    double curY = y - height/2 + style.verticalPadding;
    auto pb = ImageCache::getInstance().getCachedImage(node->imagePath, node->imgWidth, node->imgHeight);
    if (pb) {
        Gdk::Cairo::set_source_pixbuf(cr, pb, x - pb->get_width()/2.0, curY);
        cr->paint();
        curY += pb->get_height() + 5;
    }

    if (!isEditing && !node->text.empty()) {
        cr->save();
        auto cache = std::static_pointer_cast<CachedLayoutData>(node->_layoutCache);
        if (cache && cache->layout) {
            double tw = cache->width;
            double th = cache->height;
            if (!pb) curY = y - th/2.0;

            // CENTER LOGIC: Start drawing from (center_x - content_width / 2)
            // This works because we synchronized layout->set_width() to tw in MindMapDrawer.
            cr->move_to(x - tw/2.0, curY);
            cr->set_source(style.textColor);
            cache->layout->show_in_cairo_context(cr);
        }
        cr->restore();
    }
}