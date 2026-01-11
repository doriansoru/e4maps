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
