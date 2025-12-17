#ifndef MINDMAP_DRAWER_HPP
#define MINDMAP_DRAWER_HPP

#include "MindMap.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "Theme.hpp"
#include <gtkmm.h>
#include <cairomm/cairomm.h>
#include <pangomm.h>
#include <map> // For image cache
#include <tuple>
#include <algorithm> // For std::transform
#include <iostream> // For std::cerr

// Forward declaration to avoid circular dependencies if needed
// class Node; // Already defined in MindMap.hpp


// Image cache management class
class ImageCache {
private:
    std::map<std::tuple<std::string, int, int>, Glib::RefPtr<Gdk::Pixbuf>> cache;

public:
    Glib::RefPtr<Gdk::Pixbuf> getCachedImage(const std::string& path, int reqW, int reqH) {
        if (path.empty()) return {};

        // Validate the file is a supported image type
        if (!Utils::isValidImageFile(path)) {
            std::cerr << "Warning: Attempting to load unsupported image file: " << path << std::endl;
            return {};
        }

        auto key = std::make_tuple(path, reqW, reqH);
        if (cache.count(key)) return cache[key];

        try {
            auto raw = Gdk::Pixbuf::create_from_file(path);
            if (!raw) {
                std::cerr << "Error: Could not load image file: " << path << std::endl;
                return {};
            }

            int w = raw->get_width();
            int h = raw->get_height();
            double original_ratio = (double)w / h;

            int targetW = w;
            int targetH = h;
            Glib::RefPtr<Gdk::Pixbuf> scaled_pixbuf;

            if (reqW > 0 && reqH == 0) { // Fixed width, auto height
                 targetW = reqW;
                 targetH = static_cast<int>(reqW / original_ratio);
            } else if (reqH > 0 && reqW == 0) { // Fixed height, auto width
                 targetH = reqH;
                 targetW = static_cast<int>(reqH * original_ratio);
            } else if (reqW > 0 && reqH > 0) { // Both fixed: fit inside box
                double scale_factor = std::min((double)reqW / w, (double)reqH / h);
                targetW = static_cast<int>(w * scale_factor);
                targetH = static_cast<int>(h * scale_factor);
            } else { // reqW == 0 && reqH == 0 (auto-scale to default context size)
                int maxDim = 150; // Default max size for nodes if not specified
                if (w > maxDim || h > maxDim) {
                    double s = (double)maxDim / std::max(w, h);
                    targetW = static_cast<int>(w * s);
                    targetH = static_cast<int>(h * s);
                 }
            }

            if (targetW == w && targetH == h) {
                scaled_pixbuf = raw;
            } else {
                scaled_pixbuf = raw->scale_simple(targetW, targetH, Gdk::INTERP_BILINEAR);
            }

            cache[key] = scaled_pixbuf;
            return scaled_pixbuf;
        } catch(const Glib::Exception& e) {
            std::cerr << "Error loading image file '" << path << "': " << e.what() << std::endl;
            return {};
        } catch(const std::exception& e) {
            std::cerr << "Error loading image file '" << path << "': " << e.what() << std::endl;
            return {};
        } catch(...) {
            std::cerr << "Error loading image file '" << path << "'" << std::endl;
            return {};
        }
    }

    void clear() {
        cache.clear();
    }

    static ImageCache& getInstance() {
        static ImageCache instance;
        return instance;
    }
};

struct CachedLayoutData {
    Glib::RefPtr<Pango::Layout> layout;
    std::string text;
    std::string fontDesc;
};

class MindMapDrawer {
public:
    // Pre-calculate node dimensions to ensure arrows are positioned correctly
    void preCalculateNodeDimensions(std::shared_ptr<Node> node, const Theme& theme, const Cairo::RefPtr<Cairo::Context>& cr, int depth = 0) {
        if (!node) return;
        calculateNodeDimensions(node, theme, cr, depth);
        for (auto& child : node->children) {
            preCalculateNodeDimensions(child, theme, cr, depth + 1);
        }
    }

    // Calculate node dimensions without drawing
    void calculateNodeDimensions(std::shared_ptr<Node> node, const Theme& theme, const Cairo::RefPtr<Cairo::Context>& cr, int depth) {
        if (!node) return;

        NodeStyle style = theme.getStyle(depth);

        // Apply manual font override if present (same priority as in drawNode)
        if (node->overrideFont && !node->fontDesc.empty()) {
            style.fontDescription = Pango::FontDescription(node->fontDesc);
        }

        // Calculate text size using Cache
        Glib::RefPtr<Pango::Layout> layout;
        std::string currentFontDesc = style.fontDescription.to_string();
        
        std::shared_ptr<CachedLayoutData> cache;
        if (node->_layoutCache) {
             cache = std::static_pointer_cast<CachedLayoutData>(node->_layoutCache);
        }

        if (cache && cache->layout && cache->text == node->text && cache->fontDesc == currentFontDesc) {
            layout = cache->layout;
            // Update context if needed (Pango layouts are context bound)
            // Ideally we check if context changed, but mostly it's same surface type.
            // If the context scaling changed significantly, we might need to recreate, 
            // but for now assume compatible context.
            // Actually, Pango::Layout holds a ref to Pango::Context.
        } else {
            layout = Pango::Layout::create(cr);
            layout->set_text(node->text);
            layout->set_font_description(style.fontDescription);
            
            // Enable text wrapping
            layout->set_width(E4Maps::MAX_NODE_WIDTH * Pango::SCALE);
            layout->set_wrap(Pango::WRAP_WORD);
            
            // Update cache
            auto newCache = std::make_shared<CachedLayoutData>();
            newCache->layout = layout;
            newCache->text = node->text;
            newCache->fontDesc = currentFontDesc;
            node->_layoutCache = newCache;
        }
        
        int textW, textH;
        layout->get_pixel_size(textW, textH);

        double contentWidth = textW;
        double contentHeight = textH;
        double imgW = 0, imgH = 0;

        auto pb = getCachedImage(node->imagePath, node->imgWidth, node->imgHeight);
        if (pb) {
            imgW = pb->get_width(); imgH = pb->get_height();
            contentWidth = std::max(contentWidth, imgW);
            contentHeight += imgH + 5; // Padding between image and text
        }

        node->width = contentWidth + style.horizontalPadding * 2;
        node->height = contentHeight + style.verticalPadding * 2;
    }

    // Helper to load and cache images
    Glib::RefPtr<Gdk::Pixbuf> getCachedImage(const std::string& path, int reqW, int reqH) {
        return ImageCache::getInstance().getCachedImage(path, reqW, reqH);
    }

    // Helper to draw a rounded rectangle
    void rounded_rectangle(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double width, double height, double radius) {
        double degrees = M_PI / 180.0;
        cr->begin_new_sub_path();
        cr->arc(x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
        cr->arc(x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
        cr->arc(x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
        cr->arc(x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
        cr->close_path();
    }

    // Helper to calculate distance from center to rectangle boundary along an angle
    double getDistanceToRectBoundary(double width, double height, double angle) {
        double cosA = std::cos(angle);
        double sinA = std::sin(angle);
        double halfW = width / 2.0;
        double halfH = height / 2.0;

        double distX = (std::abs(cosA) > 1e-6) ? std::abs(halfW / cosA) : std::numeric_limits<double>::max();
        double distY = (std::abs(sinA) > 1e-6) ? std::abs(halfH / sinA) : std::numeric_limits<double>::max();

        return std::min(distX, distY);
    }

    // Draw organic curved arrow connection
    void drawOrganicArrow(const Cairo::RefPtr<Cairo::Context>& cr,
                         double startX, double startY,
                         double endX, double endY,
                         double nodeWidth, double nodeHeight,  // Add node dimensions
                         double width,
                         const Cairo::RefPtr<Cairo::Pattern>& color,
                         Color arrowColor,
                         int depth) {

        cr->save();
        cr->set_source(color);

        // Calculate the vector between start and end points
        double dx = endX - startX;
        double dy = endY - startY;
        double distance = std::sqrt(dx * dx + dy * dy);

        // Calculate midpoint and perpendicular offset for organic curve
        double midX = (startX + endX) / 2.0;
        double midY = (startY + endY) / 2.0;

        // Calculate perpendicular vector for organic curve effect
        double perpX = -dy / distance;
        double perpY = dx / distance;

        // Use varying offset based on depth to make branches thinner further from center
        double curveOffset = (distance / 4.0) * (1.0 - (depth * 0.1)); // Reduce curve as depth increases

        // Introduce slight randomness for organic look (consistent per session)
        // Use a deterministic pseudo-random based on coordinates to maintain consistency
        unsigned int seed = (unsigned int)((startX + startY + endX + endY) * 1000);
        double rand_offset = ((seed % 1000) / 1000.0 - 0.5) * 0.3; // Random offset between -0.15 and 0.15
        curveOffset *= (1.0 + rand_offset);

        // Calculate control point for quadratic Bézier curve
        double ctrlX = midX + perpX * curveOffset;
        double ctrlY = midY + perpY * curveOffset;

        // Draw the organic curve with varying thickness - start thick, end thin
        // Calculate where the arrow should end based on node dimensions and tangent direction
        // Approximate the node as a rectangle and find the intersection point
        
        // Calculate direction vector from end point to control point (this gives the approach direction)
        double endTangentX = 3 * (endX - ctrlX); // Approximate tangent at end point
        double endTangentY = 3 * (endY - ctrlY);
        
        // Calculate angle of approach (pointing INTO the center)
        double approachAngle = std::atan2(endTangentY, endTangentX);
        
        // We want the point on the boundary "looking back" towards the control point
        // The angle from Center to Boundary is approachAngle + M_PI (opposite to approach)
        double exitAngle = approachAngle + M_PI;
        
        double distToBoundary = getDistanceToRectBoundary(nodeWidth, nodeHeight, exitAngle);
        
        // The actual end point of the arrow curve will be at this intersection
        // Note: endX, endY are the center coordinates.
        double finalEndX = endX + std::cos(exitAngle) * distToBoundary;
        double finalEndY = endY + std::sin(exitAngle) * distToBoundary;

        // Draw the organic curve with a brush-like effect using round line caps
        // This creates a natural taper from thick at start to thin at end
        // Use the calculated final endpoint

        // Optimization: If color is opaque, draw once. If transparent, draw multiple to create core effect.
        bool isOpaque = true;
        auto solid = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(color);
        if (solid) {
            double r, g, b, a;
            solid->get_rgba(r, g, b, a);
            if (a < 0.99) isOpaque = false;
        } else {
            isOpaque = false; // Gradient or other pattern
        }

        if (isOpaque) {
             cr->set_line_width(width * 3.0); // Draw only the thickest line
             cr->set_line_cap(Cairo::LINE_CAP_ROUND);
             cr->set_source(color);
             cr->move_to(startX, startY);
             cr->curve_to(ctrlX, ctrlY, ctrlX, ctrlY, finalEndX, finalEndY);
             cr->stroke();
        } else {
            // Draw multiple strokes with decreasing width to create the gradient effect
            // Start with the largest width and work down to the smallest
            for (int i = 3; i >= 1; i--) { // Draw from thickest to thinnest
                double currentWidth = width * i; // 3x, 2x, then 1x the base width
                cr->set_line_width(currentWidth);
                cr->set_line_cap(Cairo::LINE_CAP_ROUND); // Use round caps for organic effect
                cr->set_source(color);
                cr->move_to(startX, startY);
                // Use the calculated endpoint for the curve
                cr->curve_to(ctrlX, ctrlY, ctrlX, ctrlY, finalEndX, finalEndY);
                cr->stroke();
            }
        }

        // Calculate tangent at the end of the curve for arrowhead direction
        // Use the final end point for calculating tangent at the end
        double tangentX = 3 * (finalEndX - ctrlX); // Tangent at actual end point
        double tangentY = 3 * (finalEndY - ctrlY);
        double arrowAngle = std::atan2(tangentY, tangentX);

        // Calculate arrowhead size based on connection width and depth
        double arrowSize = std::max(width * 3.0, 8.0); // Adjusted for curved arrow

        // Draw the arrowhead at the calculated intersection point
        cr->save();
        cr->translate(finalEndX, finalEndY);
        cr->rotate(arrowAngle);
        // Make arrowhead size proportional to line width, but quadrupled for visibility
        double arrowHalfWidth = width * 12.0; // Arrowhead width quadrupled (3.0 * 4)
        double arrowLength = width * 16.0; // Arrowhead length quadrupled (4.0 * 4)
        cr->move_to(0, 0);
        cr->line_to(-arrowLength, -arrowHalfWidth);
        cr->line_to(-arrowLength * 0.5, 0);
        cr->line_to(-arrowLength, arrowHalfWidth);
        cr->close_path();

        // Use the arrow color for the arrowhead
        cr->set_source_rgb(arrowColor.r, arrowColor.g, arrowColor.b);
        cr->fill();

        // Add border to the arrowhead
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->set_line_width(1.0);
        cr->stroke();
        cr->restore();

        cr->restore();
    }

    void drawArrow(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y, double angle, double size, Color color) {
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

    void drawNode(const Cairo::RefPtr<Cairo::Context>& cr, std::shared_ptr<Node> node, int depth, const Theme& theme, std::shared_ptr<Node> selectedNode = nullptr, const std::vector<std::shared_ptr<Node>>& selectedNodes = {}) {
        if (!node) return;

        NodeStyle style = theme.getStyle(depth);

        // --- MANUAL OVERRIDES (Highest Priority) ---
        if (node->overrideTextColor) {
            style.textColor = Cairo::SolidPattern::create_rgb(node->textColor.r, node->textColor.g, node->textColor.b);
        }
        if (node->overrideFont && !node->fontDesc.empty()) {
            style.fontDescription = Pango::FontDescription(node->fontDesc);
        }

        // Draw connections first (so they are behind nodes)
        for (auto& child : node->children) {
            cr->save();
            
            // Determine connection color for this specific child
            Cairo::RefPtr<Cairo::Pattern> connColor = style.connectionColor;
            if (child->overrideColor) {
                connColor = Cairo::SolidPattern::create_rgb(child->color.r, child->color.g, child->color.b);
            }
            
            cr->set_source(connColor);
            // Thinner, more elegant lines
            cr->set_line_width(style.connectionWidth); // Using themed connection width
            cr->set_line_cap(Cairo::LINE_CAP_ROUND);

            if (style.connectionDash) {
                std::vector<double> dashes = {6.0, 3.0};
                cr->set_dash(dashes, 0.0);
            }

            // Calculate common points for both connection types (needed for annotations)
            double dx = child->x - node->x;
            double dy = child->y - node->y;
            double dist = std::sqrt(dx*dx + dy*dy);

            // Smoother bezier curves with adjustable tension (for annotations positioning)
            double cpDist = dist * 0.4;

            // Calculate angle but clamp it to avoid extreme loops for nearby nodes
            double geoAngle = std::atan2(dy, dx);

            // Initial/Final offsets to make lines start/end from node edges roughly
            // Simple approximation: start a bit outside the center

            double p0x = node->x; double p0y = node->y;
            double p3x = child->x; double p3y = child->y;

            // Control points:
            // P1 projects out from parent
            // P2 projects out from child (inverse direction)
            // Use standard horizontal/radial projection logic based on layout type ideally,
            // but here we stick to radial-ish logic

            double p1x = p0x + cpDist * std::cos(geoAngle);
            double p1y = p0y + cpDist * std::sin(geoAngle);
            double p2x = p3x - cpDist * std::cos(geoAngle);
            double p2y = p3y - cpDist * std::sin(geoAngle);

            // Check connection type and draw accordingly
            if (style.connectionType == 1) { // Organic arrow style
                // Draw organic curved arrow connection
                drawOrganicArrow(cr, node->x, node->y, child->x, child->y, child->width, child->height, style.connectionWidth, connColor, child->color, depth);
            } else { // Traditional arrow style
                cr->move_to(p0x, p0y);
                cr->curve_to(p1x, p1y, p2x, p2y, p3x, p3y);
                cr->stroke();

                // Arrow logic remains similar
                double endTangentX = 3 * (p3x - p2x);
                double endTangentY = 3 * (p3y - p2y);
                double arrowAngle = std::atan2(endTangentY, endTangentX);

                // Use precise intersection with the node's bounding box
                // arrowAngle points INTO the center. We want to back off along the line.
                // The direction FROM Center TO Boundary is arrowAngle + M_PI.
                double exitAngle = arrowAngle + M_PI;
                double distToBoundary = getDistanceToRectBoundary(child->width, child->height, exitAngle);

                double arrowSize = std::max(10.0, 18.0 - depth * 1.2);
                
                // Position the arrow tip exactly on the boundary
                // We move from Center (p3x, p3y) in the direction of exitAngle by distToBoundary.
                double tipX = p3x + std::cos(exitAngle) * distToBoundary;
                double tipY = p3y + std::sin(exitAngle) * distToBoundary;

                Color arrowColor = {0, 0, 0};
                auto solidPattern = Cairo::RefPtr<Cairo::SolidPattern>::cast_dynamic(connColor); // Use the effective connection color
                if (solidPattern) {
                    double r, g, b, a;
                    solidPattern->get_rgba(r, g, b, a);
                    arrowColor.r = r;
                    arrowColor.g = g;
                    arrowColor.b = b;
                } else {
                    // Fallback to node color if pattern is not solid or unavailable
                    arrowColor = child->color;
                }

                drawArrow(cr, tipX, tipY, arrowAngle, arrowSize, arrowColor);
            }

            // Annotations (Text/Image on line) - works for both connection types
            if (!child->connText.empty() || !child->connImagePath.empty()) {
                double mx, my; // midpoint for annotation
                double tangent_angle; // angle for rotation

                if (style.connectionType == 1) { // Organic curve style
                    // For organic curve style, calculate midpoint and angle based on the actual drawn curve
                    // Calculate vector between start and end points
                    double dx = child->x - node->x;
                    double dy = child->y - node->y;
                    double distance = std::sqrt(dx * dx + dy * dy);

                    // Calculate perpendicular vector for organic curve
                    double perpX = -dy / distance;
                    double perpY = dx / distance;

                    // Use same curve offset logic as in drawOrganicArrow
                    double curveOffset = (distance / 4.0) * (1.0 - (depth * 0.1)); // Reduce curve as depth increases
                    unsigned int seed = (unsigned int)((node->x + node->y + child->x + child->y) * 1000);
                    double rand_offset = ((seed % 1000) / 1000.0 - 0.5) * 0.3;
                    curveOffset *= (1.0 + rand_offset);

                    // Calculate control point for quadratic Bézier curve (same as in drawOrganicArrow)
                    double midX = (node->x + child->x) / 2.0;
                    double midY = (node->y + child->y) / 2.0;
                    double ctrlX = midX + perpX * curveOffset;
                    double ctrlY = midY + perpY * curveOffset;

                    // Calculate point and tangent at t=0.5 along the quadratic Bézier curve
                    // But also consider the text size to adjust positioning if needed
                    double t = 0.5;

                    // Get font for connection text to consider its size
                    Pango::FontDescription conn_font;
                    if (child->overrideConnFont && !child->connFontDesc.empty()) {
                        conn_font = Pango::FontDescription(child->connFontDesc);
                    } else {
                        conn_font = style.connectionFontDescription;
                    }

                    // Create a layout to measure text dimensions
                    auto layout = Pango::Layout::create(cr);
                    layout->set_text(child->connText);
                    layout->set_font_description(conn_font);

                    int textW, textH;
                    layout->get_pixel_size(textW, textH);

                    // Adjust t value based on text length and distance between nodes
                    // For longer text relative to distance, move it slightly to avoid edges
                    double textRatio = static_cast<double>(textW) / std::max(distance * 0.5, 1.0); // Avoid division by zero
                    if (textRatio > 0.8) {
                        // If text is quite long relative to the connection distance, move it closer to center
                        t = 0.5; // Keep as is, but we could adjust if needed
                    }

                    // Calculate point along the quadratic Bézier curve
                    mx = (1-t)*(1-t)*node->x + 2*(1-t)*t*ctrlX + t*t*child->x;
                    my = (1-t)*(1-t)*node->y + 2*(1-t)*t*ctrlY + t*t*child->y;

                    // Calculate tangent at t for rotation
                    // Derivative of quadratic Bézier: B'(t) = 2*(1-t)*(P1-P0) + 2*t*(P2-P1)
                    double tangentX = 2*(1-t)*(ctrlX - node->x) + 2*t*(child->x - ctrlX);
                    double tangentY = 2*(1-t)*(ctrlY - node->y) + 2*t*(child->y - ctrlY);
                    tangent_angle = std::atan2(tangentY, tangentX);
                } else { // Traditional arrow style (Bezier curve)
                    double t = 0.5;
                    // Bezier point at t=0.5
                    mx = (1-t)*(1-t)*(1-t)*p0x + 3*(1-t)*(1-t)*t*p1x + 3*(1-t)*t*t*p2x + t*t*t*p3x;
                    my = (1-t)*(1-t)*(1-t)*p0y + 3*(1-t)*(1-t)*t*p1y + 3*(1-t)*t*t*p2y + t*t*t*p3y;

                    // Tangent for rotation
                    double tangentX = 3*(1-t)*(1-t)*(p1x-p0x) + 6*(1-t)*t*(p2x-p1x) + 3*t*t*(p3x-p2x);
                    double tangentY = 3*(1-t)*(1-t)*(p1y-p0y) + 6*(1-t)*t*(p2y-p1y) + 3*t*t*(p3y-p2y);
                    tangent_angle = std::atan2(tangentY, tangentX);
                }

                cr->save();
                cr->translate(mx, my);
                cr->rotate(tangent_angle);

                if (std::abs(tangent_angle) > M_PI/2) {
                    cr->rotate(M_PI);
                }

                // ... (Content drawing logic remains similar, simplified for brevity)
                double totalContentWidth = 0; 
                int tw = 0, th = 0;
                
                 if (!child->connImagePath.empty()) {
                    auto pb = getCachedImage(child->connImagePath, 24, 24); 
                    if(pb) totalContentWidth += pb->get_width();
                }
                
                if (!child->connText.empty()) {
                    Pango::FontDescription conn_font;
                    if (child->overrideConnFont && !child->connFontDesc.empty()) {
                        conn_font = Pango::FontDescription(child->connFontDesc);
                    } else {
                        conn_font = style.connectionFontDescription;
                    }

                    auto layout = Pango::Layout::create(cr);
                    layout->set_text(child->connText);
                    layout->set_font_description(conn_font);
                    layout->get_pixel_size(tw, th);
                    totalContentWidth += tw; 
                }

                double currentX = -totalContentWidth / 2.0; 
                double padding = 2.0;
                
                if (!child->connImagePath.empty()) {
                     auto pb = getCachedImage(child->connImagePath, 24, 24); 
                     if (pb) {
                         Gdk::Cairo::set_source_pixbuf(cr, pb, currentX, -pb->get_height() - padding);
                         cr->paint();
                         currentX += pb->get_width(); 
                     }
                }

                if (!child->connText.empty()) {
                    Pango::FontDescription conn_font;
                    if (child->overrideConnFont && !child->connFontDesc.empty()) {
                        conn_font = Pango::FontDescription(child->connFontDesc);
                    } else {
                        conn_font = style.connectionFontDescription;
                    }
                    // Small background for readability
                    cr->set_source_rgba(1, 1, 1, 0.8);
                    rounded_rectangle(cr, currentX - 2, -th - padding - 2, tw + 4, th + 4, 3.0);
                    cr->fill();
                    
                    cr->set_source_rgb(0.3, 0.3, 0.3);
                    auto layout = Pango::Layout::create(cr);
                    layout->set_text(child->connText);
                    layout->set_font_description(conn_font);
                    cr->move_to(currentX, -th - padding); 
                    layout->show_in_cairo_context(cr);
                }
                cr->restore(); 
            }
            cr->restore();
            drawNode(cr, child, depth + 1, theme, selectedNode, selectedNodes); // RECURSIVE CALL UPDATE
        }

        // --- DRAW NODE ---
        cr->save();
        // Dimensions should already be calculated by preCalculateNodeDimensions
        // or calculateNodeDimensions.
        // We still need a Pango Layout to draw the text correctly,
        // and image dimensions for positioning.
        
        Glib::RefPtr<Pango::Layout> layout;
        std::shared_ptr<CachedLayoutData> cache;
        if (node->_layoutCache) {
             cache = std::static_pointer_cast<CachedLayoutData>(node->_layoutCache);
        }
        
        std::string currentFontDesc = style.fontDescription.to_string();

        if (cache && cache->layout && cache->text == node->text && cache->fontDesc == currentFontDesc) {
            layout = cache->layout;
        } else {
             // Fallback if cache invalid (e.g. if preCalculate wasn't called or props changed)
            layout = Pango::Layout::create(cr);
            layout->set_text(node->text);
            layout->set_font_description(style.fontDescription);
            layout->set_width(E4Maps::MAX_NODE_WIDTH * Pango::SCALE);
            layout->set_wrap(Pango::WRAP_WORD);
            
             // Update cache
            auto newCache = std::make_shared<CachedLayoutData>();
            newCache->layout = layout;
            newCache->text = node->text;
            newCache->fontDesc = currentFontDesc;
            node->_layoutCache = newCache;
        }
        
        int textW, textH;
        layout->get_pixel_size(textW, textH); // Get actual text dimensions for drawing

        double imgW = 0, imgH = 0;
        auto pb = getCachedImage(node->imagePath, node->imgWidth, node->imgHeight);
        if (pb) {
            imgW = pb->get_width(); imgH = pb->get_height();
        }

        double pad = style.horizontalPadding; // Use themed padding
        double totalW = node->width;
        double totalH = node->height; // Use themed padding
        double cornerRadius = style.cornerRadius; // Use themed corner radius
        double boxX = node->x - totalW/2;
        double boxY = node->y - totalH/2;

        // --- OPTIMIZATION: Frustum Culling ---
        // Check if the node is within the visible clip area
        double clipX1, clipY1, clipX2, clipY2;
        cr->get_clip_extents(clipX1, clipY1, clipX2, clipY2);

        // Simple AABB intersection check
        // Node box is [boxX, boxY, totalW, totalH] + some margin for shadow/border
        double margin = 20.0; // generous margin
        bool isVisible = (boxX + totalW + margin >= clipX1) &&
                         (boxX - margin <= clipX2) &&
                         (boxY + totalH + margin >= clipY1) &&
                         (boxY - margin <= clipY2);

        if (isVisible) {
            // 1. Draw Shadow
            cr->save();
            cr->set_source(style.shadowColor); // Use themed shadow color
            rounded_rectangle(cr, boxX + style.shadowOffsetX, boxY + style.shadowOffsetY, totalW, totalH, cornerRadius);
            cr->fill();
            cr->restore();

            // 2. Draw Node Background (Gradient or Solid)
            bool isNodeSelected = (node == selectedNode) ||
                                 (!selectedNodes.empty() &&
                                  std::find(selectedNodes.begin(), selectedNodes.end(), node) != selectedNodes.end());

            if (isNodeSelected) {
                cr->set_source(style.backgroundHoverColor); // Use themed hover color if selected
            } else {
                cr->set_source(style.backgroundColor); // Use themed background color
            }
            rounded_rectangle(cr, boxX, boxY, totalW, totalH, cornerRadius);
            cr->fill_preserve(); // Keep path for stroke

            // 3. Draw Border
            if (isNodeSelected) {
                cr->set_source_rgb(0.2, 0.6, 1.0); // Highlight color remains hardcoded for now
                cr->set_line_width(2.5);
            } else {
                cr->set_source(style.borderColor); // Use themed border color
                cr->set_line_width(style.borderWidth); // Use themed border width
            }
            cr->stroke();

            // 4. Draw Content
            if (pb) {
                Gdk::Cairo::set_source_pixbuf(cr, pb, node->x - imgW/2, boxY + style.verticalPadding);
                cr->paint();
            }

            if (isNodeSelected) {
                cr->set_source(style.textHoverColor);
            } else {
                cr->set_source(style.textColor); // Use themed text color
            }
            double textY = boxY + style.verticalPadding + ((pb) ? imgH + 5 : 0);
            cr->move_to(node->x - textW/2, textY);
            layout->show_in_cairo_context(cr);
        }
        
        cr->restore();
    }

    // Clear the image cache
    static void clearImageCache() {
        ImageCache::getInstance().clear();
    }

private:
    Theme currentTheme;
};

#endif // MINDMAP_DRAWER_HPP

