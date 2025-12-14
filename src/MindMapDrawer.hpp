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
#include <algorithm> // For std::transform
#include <iostream> // For std::cerr

// Forward declaration to avoid circular dependencies if needed
// class Node; // Already defined in MindMap.hpp


// Image cache management class
class ImageCache {
private:
    std::map<std::string, Glib::RefPtr<Gdk::Pixbuf>> cache;

public:
    Glib::RefPtr<Gdk::Pixbuf> getCachedImage(const std::string& path, int reqW, int reqH) {
        if (path.empty()) return {};

        // Validate the file is a supported image type
        if (!Utils::isValidImageFile(path)) {
            std::cerr << "Warning: Attempting to load unsupported image file: " << path << std::endl;
            return {};
        }

        std::string key = path + "_" + std::to_string(reqW) + "x" + std::to_string(reqH);
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

        // Calculate text size
        auto layout = Pango::Layout::create(cr);
        layout->set_text(node->text);
        Pango::FontDescription font(style.fontDescription);
        layout->set_font_description(font);
        
        // Enable text wrapping
        layout->set_width(E4Maps::MAX_NODE_WIDTH * Pango::SCALE);
        layout->set_wrap(Pango::WRAP_WORD);
        
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

            double dx = child->x - node->x;
            double dy = child->y - node->y;
            double dist = std::sqrt(dx*dx + dy*dy);
            
            // Smoother bezier curves with adjustable tension
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

            cr->move_to(p0x, p0y);
            cr->curve_to(p1x, p1y, p2x, p2y, p3x, p3y);
            cr->stroke();

            // Arrow logic remains similar
            double endTangentX = 3 * (p3x - p2x);
            double endTangentY = 3 * (p3y - p2y);
            double arrowAngle = std::atan2(endTangentY, endTangentX);

            double nodeRadius = std::max(child->width, child->height) / 2.0 + 8.0;
            double arrowSize = std::max(10.0, 18.0 - depth * 1.2);
            double totalOffset = nodeRadius + arrowSize * 0.8;

            double offsetX = std::cos(arrowAngle) * totalOffset;
            double offsetY = std::sin(arrowAngle) * totalOffset;
            
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
            
            drawArrow(cr, p3x - offsetX, p3y - offsetY, arrowAngle, arrowSize, arrowColor);
            
            // Annotations (Text/Image on line)
            if (!child->connText.empty() || !child->connImagePath.empty()) {
                double t = 0.5; 
                // Bezier point at t=0.5
                double mx = (1-t)*(1-t)*(1-t)*p0x + 3*(1-t)*(1-t)*t*p1x + 3*(1-t)*t*t*p2x + t*t*t*p3x;
                double my = (1-t)*(1-t)*(1-t)*p0y + 3*(1-t)*(1-t)*t*p1y + 3*(1-t)*t*t*p2y + t*t*t*p3y;
                
                // Tangent for rotation
                double tangentX = 3*(1-t)*(1-t)*(p1x-p0x) + 6*(1-t)*t*(p2x-p1x) + 3*t*t*(p3x-p2x);
                double tangentY = 3*(1-t)*(1-t)*(p1y-p0y) + 6*(1-t)*t*(p2y-p1y) + 3*t*t*(p3y-p2y);
                double tangent_angle = std::atan2(tangentY, tangentX);

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
                    auto layout = Pango::Layout::create(cr);
                    layout->set_text(child->connText);
                    Pango::FontDescription font("Sans Italic 9");
                    layout->set_font_description(font);
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
                    // Small background for readability
                    cr->set_source_rgba(1, 1, 1, 0.8);
                    rounded_rectangle(cr, currentX - 2, -th - padding - 2, tw + 4, th + 4, 3.0);
                    cr->fill();
                    
                    cr->set_source_rgb(0.3, 0.3, 0.3);
                    auto layout = Pango::Layout::create(cr);
                    layout->set_text(child->connText);
                    Pango::FontDescription font("Sans Italic 9");
                    layout->set_font_description(font);
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
        
        auto layout = Pango::Layout::create(cr);
        layout->set_text(node->text);
        Pango::FontDescription font(style.fontDescription); // Use themed font
        layout->set_font_description(font);
        
        // Enable text wrapping
        layout->set_width(E4Maps::MAX_NODE_WIDTH * Pango::SCALE);
        layout->set_wrap(Pango::WRAP_WORD);
        
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

        cr->set_source(style.textColor); // Use themed text color
        double textY = boxY + style.verticalPadding + ((pb) ? imgH + 5 : 0);
        cr->move_to(node->x - textW/2, textY);
        layout->show_in_cairo_context(cr);
        
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

