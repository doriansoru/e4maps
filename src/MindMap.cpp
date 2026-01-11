#include "MindMap.hpp"
#include "Translation.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "MapSerializer.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <random>
#include <map>

Color Color::random() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return {dis(gen), dis(gen), dis(gen)};
}

int Node::generateId() {
    static int _nextId = 0;
    return ++_nextId;
}

Node::Node(const std::string& t, Color c) : text(t), color(c), id(generateId()) {
    // Default font
    fontDesc = "";
    connFontDesc = "";
}

void Node::addChild(std::shared_ptr<Node> child) {
    child->parent = weak_from_this();
    children.push_back(child);
}

void Node::removeChild(std::shared_ptr<Node> child) {
    auto it = std::remove(children.begin(), children.end(), child);
    children.erase(it, children.end());
}

bool Node::isRoot() const { return parent.expired(); }

bool Node::contains(double px, double py) const {
    double margin = E4Maps::NODE_MARGIN;
    return (px >= x - width/2 - margin && px <= x + width/2 + margin &&
            py >= y - height/2 - margin && py <= y + height/2 + margin);
}





MindMap::MindMap(const std::string& rootText) {
    root = std::make_shared<Node>(rootText, Color{0.0, 0.0, 0.0});
}

MindMap::MindMap() {}

std::shared_ptr<Node> MindMap::hitTestRecursive(std::shared_ptr<Node> node, double x, double y) {
    if (!node) return nullptr;

    // Check children first (reverse order to pick the one drawn last/on top)
    for (auto it = node->children.rbegin(); it != node->children.rend(); ++it) {
        auto found = hitTestRecursive(*it, x, y);
        if (found) return found;
    }

    // Check the node itself last
    if (node->contains(x, y)) return node;

    return nullptr;
}

std::shared_ptr<Node> MindMap::hitTest(double x, double y) {
    return hitTestRecursive(root, x, y);
}

// Helper to calculate distance from point to Quadratic Bezier
// P = (1-t)^2 * P0 + 2(1-t)t * P1 + t^2 * P2
// We approximate by checking distance to sample points along the curve.
// Exact mathematical solution involves solving a cubic equation which is complex.
double pointToBezierDistance(double px, double py, 
                             double p0x, double p0y, 
                             double p1x, double p1y, 
                             double p2x, double p2y) {
    double minDstSq = std::numeric_limits<double>::max();
    
    // Sample 20 points along the curve
    const int STEPS = 20;
    for (int i = 0; i <= STEPS; ++i) {
        double t = static_cast<double>(i) / STEPS;
        double u = 1.0 - t;
        double cx = u*u*p0x + 2*u*t*p1x + t*t*p2x;
        double cy = u*u*p0y + 2*u*t*p1y + t*t*p2y;
        
        double dstSq = (px-cx)*(px-cx) + (py-cy)*(py-cy);
        if (dstSq < minDstSq) {
            minDstSq = dstSq;
        }
    }
    return std::sqrt(minDstSq);
}

Connection* MindMap::hitTestConnection(double x, double y, double tolerance) {
    // Increase tolerance slightly for better UX
    double clickTolerance = tolerance + 5.0; 

    for (auto& conn : connections) {
        if (!conn.from || !conn.to) continue;
        
        double startX = conn.from->x;
        double startY = conn.from->y;
        double endX = conn.to->x;
        double endY = conn.to->y;
        
        double dx = endX - startX;
        double dy = endY - startY;
        double distance = std::sqrt(dx * dx + dy * dy);
        
        if (distance < 0.1) continue;

        // Calculate actual depth of the 'from' node to match Drawer logic
        int depth = 0;
        auto p = conn.from->parent.lock();
        while(p) { 
            depth++; 
            p = p->parent.lock(); 
        }

        double midX = (startX + endX) / 2.0;
        double midY = (startY + endY) / 2.0;
        double perpX = -dy / distance;
        double perpY = dx / distance;

        // MATCH MindMapDrawer::drawOrganicArrow logic exactly
        double curveOffset = (distance / 4.0) * (1.0 - (depth * 0.1));
        unsigned int seed = (unsigned int)((startX + startY + endX + endY) * 1000);
        double rand_offset = ((seed % 1000) / 1000.0 - 0.5) * 0.3;
        curveOffset *= (1.0 + rand_offset);

        double ctrlX = midX + perpX * curveOffset;
        double ctrlY = midY + perpY * curveOffset;
        
        // Increase sampling steps from 20 to 50 for pixel-perfect precision
        const int STEPS = 50;
        double minDstSq = std::numeric_limits<double>::max();
        for (int i = 0; i <= STEPS; ++i) {
            double t = static_cast<double>(i) / STEPS;
            double u = 1.0 - t;
            // Quadratic Bezier formula
            double cx = u*u*startX + 2*u*t*ctrlX + t*t*endX;
            double cy = u*u*startY + 2*u*t*ctrlY + t*t*endY;
            
            double dstSq = (x-cx)*(x-cx) + (y-cy)*(y-cy);
            if (dstSq < minDstSq) minDstSq = dstSq;
        }
        
        if (std::sqrt(minDstSq) <= clickTolerance) {
            return &conn;
        }
    }
    return nullptr;
}



std::shared_ptr<Node> cloneNodeTree(std::shared_ptr<Node> original) {
    if (!original) return nullptr;
    auto copy = std::make_shared<Node>(original->text, original->color);
    // Copy all properties
    copy->id = original->id; // IMPORTANT: Keep same ID for mapping back!
    copy->fontDesc = original->fontDesc;
    copy->textColor = original->textColor;
    copy->imagePath = original->imagePath;
    copy->imgWidth = original->imgWidth;
    copy->imgHeight = original->imgHeight;
    copy->connText = original->connText;
    copy->connImagePath = original->connImagePath;
    copy->connFontDesc = original->connFontDesc;
    copy->x = original->x;
    copy->y = original->y;
    copy->width = original->width;
    copy->height = original->height;
    copy->angle = original->angle;
    copy->manualPosition = original->manualPosition;

    // Copy overrides
    copy->overrideColor = original->overrideColor;
    copy->overrideTextColor = original->overrideTextColor;
    copy->overrideFont = original->overrideFont;
    copy->overrideConnFont = original->overrideConnFont;

    for (const auto& child : original->children) {
        auto childCopy = cloneNodeTree(child);
        copy->addChild(childCopy);
    }
    return copy;
}

void MindMap::addConnection(std::shared_ptr<Node> from, std::shared_ptr<Node> to) {
    // Check if connection already exists to avoid duplicates
    for (const auto& conn : connections) {
        if (conn.from == from && conn.to == to) {
            return; // Connection already exists
        }
    }

    connections.emplace_back(from, to);
}

void MindMap::removeConnection(size_t index) {
    if (index < connections.size()) {
        connections.erase(connections.begin() + index);
    }
}

void MindMap::removeConnection(std::shared_ptr<Node> from, std::shared_ptr<Node> to) {
    auto it = std::find_if(connections.begin(), connections.end(),
        [from, to](const Connection& conn) {
            return conn.from == from && conn.to == to;
        });

    if (it != connections.end()) {
        connections.erase(it);
    }
}
