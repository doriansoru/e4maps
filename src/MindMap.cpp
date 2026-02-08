#include "MindMap.hpp"
#include "MindMapUtils.hpp"
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

E4Color E4Color::random() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return {dis(gen), dis(gen), dis(gen)};
}

int Connection::generateId() {

    static int currentId = 0;

    return ++currentId;

}



Connection::Connection(std::shared_ptr<Node> f, std::shared_ptr<Node> t)

    : from(f), to(t), color({0.0, 0.0, 0.0}), id(generateId()) {}



int Node::generateId() {

    static int _nextId = 0;

    return ++_nextId;

}



Node::Node(const std::string& t, E4Color c) : text(t), color(c), id(generateId()) {

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

    root = std::make_shared<Node>(rootText, E4Color{0.8, 0.2, 0.2}); // Root is red

    root->overrideColor = true;

}



MindMap::MindMap() {

    root = std::make_shared<Node>("Central Topic", E4Color{0.8, 0.2, 0.2});

    root->overrideColor = true;

}



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

    if (!root) return nullptr;

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



std::shared_ptr<Connection> MindMap::hitTestConnection(double x, double y, double tolerance) {

    // Increase tolerance slightly for better UX

    double clickTolerance = tolerance + 5.0; 



    for (auto& conn : connections) {

        if (!conn->from || !conn->to) continue;

        

        double startX = conn->from->x;

        double startY = conn->from->y;

        double endX = conn->to->x;

        double endY = conn->to->y;

        

        double dx = endX - startX;

        double dy = endY - startY;

        double distance = std::sqrt(dx * dx + dy * dy);

        

        if (distance < 0.1) continue;



        // Calculate actual depth of the 'from' node to match Drawer logic

        int depth = 0;

        auto p = conn->from->parent.lock();

                while(p) { 

                    depth++; 

                    p = p->parent.lock(); 

                }

        

                double ctrlX, ctrlY;

                MindMapUtils::calculateOrganicBezierControlPoint(startX, startY, endX, endY, depth, ctrlX, ctrlY);

                

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

            return conn;

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



void MindMap::addConnection(std::shared_ptr<Connection> connection) {

    connections.push_back(connection);

}



std::shared_ptr<Connection> MindMap::addConnection(std::shared_ptr<Node> from, std::shared_ptr<Node> to) {

    // Check if connection already exists to avoid duplicates

    for (const auto& conn : connections) {

        if (conn->from == from && conn->to == to) {

            return conn; // Connection already exists

        }

    }

    

    auto conn = std::make_shared<Connection>(from, to);

    connections.push_back(conn);

    return conn;

}



void MindMap::removeConnection(std::shared_ptr<Connection> connection) {

    auto it = std::remove(connections.begin(), connections.end(), connection);

    if (it != connections.end()) {

        connections.erase(it, connections.end());

    }

}



void MindMap::removeConnection(std::shared_ptr<Node> from, std::shared_ptr<Node> to) {

    auto it = std::remove_if(connections.begin(), connections.end(),

                             [&](const std::shared_ptr<Connection>& conn) {

                                 return conn->from == from && conn->to == to;

                             });

    if (it != connections.end()) {

        connections.erase(it, connections.end());

    }

}
