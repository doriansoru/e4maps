#ifndef DRAWING_CONTEXT_HPP
#define DRAWING_CONTEXT_HPP

#include <cairomm/cairomm.h>
#include <gtkmm.h>
#include <thread>
#include <atomic>
#include <functional>
#include <map>
#include <glibmm/dispatcher.h>
#include "MindMap.hpp"
#include "MindMapDrawer.hpp"
#include "Utils.hpp"
#include "Constants.hpp"
#include "LayoutAlgorithm.hpp"
#include "MindMapUtils.hpp"

struct Viewport {
    double offsetX = 0.0;
    double offsetY = 0.0;
    double scale = 1.0;
    
    Viewport() = default;
    Viewport(double x, double y, double s) : offsetX(x), offsetY(y), scale(s) {}
};

class DrawingContext : public sigc::trackable {
private:
    Viewport viewport;
    std::shared_ptr<MindMap> map;
    std::shared_ptr<Node> selectedNode;
    MindMapDrawer drawer;
    
    // Threading
    Glib::Dispatcher m_dispatcher;
    std::thread m_workerThread;
    std::atomic<bool> m_isCalculating{false};
    std::shared_ptr<Node> m_calculatedRoot;
    std::function<void()> m_redrawCallback;
    bool m_dimensions_dirty = true; // New dirty flag

public:
    DrawingContext(std::shared_ptr<MindMap> m) : map(m), selectedNode(m->root) {
        m_dispatcher.connect(sigc::mem_fun(*this, &DrawingContext::onLayoutFinished));
    }
    
    ~DrawingContext() {
        if (m_workerThread.joinable()) {
            m_workerThread.detach();
        }
    }
    
    void setRedrawCallback(std::function<void()> cb) {
        m_redrawCallback = cb;
    }

    void setMap(std::shared_ptr<MindMap> m) {
        map = m;
        selectedNode = m->root;
        viewport = Viewport(); 
        if (map && map->root) {
             LayoutAlgorithms::calculateImprovedRadialLayout(map->root, 0, 0, 0, 2*M_PI, 0);
        }
        m_dimensions_dirty = true; // Mark dimensions as dirty when map changes
        invalidateLayout(); 
    }
    
    void invalidateLayout() {
        if (!map || !map->root) return;
        if (m_isCalculating) return; 

        m_isCalculating = true;
        
        auto clone = cloneNodeTree(map->root);
        int w = 4096;
        int h = 4096;

        if (m_workerThread.joinable()) m_workerThread.join();

        m_workerThread = std::thread([this, clone, w, h]() {
            LayoutAlgorithms::calculateForceDirectedLayout(clone, w, h);
            this->m_calculatedRoot = clone;
            this->m_dispatcher.emit();
        });
    }
    
    void onLayoutFinished() {
        if (m_workerThread.joinable()) m_workerThread.join();
        m_isCalculating = false;

        if (m_calculatedRoot && map && map->root) {
            applyLayout(map->root, m_calculatedRoot);
            m_calculatedRoot.reset();
            
            if (m_redrawCallback) m_redrawCallback();
        }
    }
    
    void applyLayout(std::shared_ptr<Node> original, std::shared_ptr<Node> computed) {
        std::map<int, std::pair<double, double>> posMap;
        collectPositions(computed, posMap);
        applyPositionsRecursive(original, posMap);
    }
    
    void collectPositions(std::shared_ptr<Node> node, std::map<int, std::pair<double, double>>& map) {
        if(!node) return;
        map[node->id] = {node->x, node->y};
        for(auto& child : node->children) collectPositions(child, map);
    }

    void applyPositionsRecursive(std::shared_ptr<Node> node, const std::map<int, std::pair<double, double>>& map) {
        if(!node) return;
        auto it = map.find(node->id);
        if (it != map.end()) {
            if (!node->manualPosition) { 
                node->x = it->second.first;
                node->y = it->second.second;
            }
        }
        for(auto& child : node->children) applyPositionsRecursive(child, map);
    }

    void setSelectedNode(std::shared_ptr<Node> node) {
        selectedNode = node;
    }

    std::shared_ptr<Node> getSelectedNode() const { return selectedNode; }

    const Viewport& getViewport() const { return viewport; }

    void setViewport(const Viewport& vp) { viewport = vp; }

    void translate(double dx, double dy) {
        viewport.offsetX += dx;
        viewport.offsetY += dy;
    }

    void scale(double factor) {
        viewport.scale *= factor;
        viewport.scale = std::max(E4Maps::MIN_ZOOM, std::min(E4Maps::MAX_ZOOM, viewport.scale));
    }

    void setScale(double scale) {
        viewport.scale = std::max(E4Maps::MIN_ZOOM, std::min(E4Maps::MAX_ZOOM, scale));
    }

    void resetView() {
        viewport.offsetX = 0.0;
        viewport.offsetY = 0.0;
        viewport.scale = 1.0;
    }

    void resetViewToCenter(int width, int height) {
        centerView(width, height);
    }

    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height) {
        cr->save();
        cr->translate(width/2.0 + viewport.offsetX, height/2.0 + viewport.offsetY);
        cr->scale(viewport.scale, viewport.scale);

        cr->set_source_rgb(1, 1, 1);
        cr->paint();

        if (!map || !map->root) {
            cr->restore();
            return true;
        }

        if (m_dimensions_dirty) {
            drawer.preCalculateNodeDimensions(map->root);
            m_dimensions_dirty = false;
        }
        
        // Drawing is now decoupled from heavy layout calculation.
        // Layout happens in background thread.

        drawer.drawNode(cr, map->root, 0, selectedNode);

        cr->restore();
        return true;
    }



    void centerView(int width, int height) {
        double minX, minY, maxX, maxY;
        if (MindMapUtils::calculateMapBounds(map->root, minX, minY, maxX, maxY)) {
            double contentCenterX = (minX + maxX) / 2.0;
            double contentCenterY = (minY + maxY) / 2.0;
            double contentWidth = maxX - minX; double contentHeight = maxY - minY;
            double scaleX = width / (contentWidth + 100);
            double scaleY = height / (contentHeight + 100);
            double newScale = std::min(scaleX, scaleY);
            newScale = std::max(0.1, std::min(newScale, 2.0));
            viewport.scale = newScale;
            viewport.offsetX = -contentCenterX * newScale;
            viewport.offsetY = -contentCenterY * newScale;
        } else {
            resetView();
        }
    }

    std::pair<double, double> screenToWorld(double screenX, double screenY, int width, int height) const {
        double worldX = (screenX - width/2.0 - viewport.offsetX) / viewport.scale;
        double worldY = (screenY - height/2.0 - viewport.offsetY) / viewport.scale;
        return {worldX, worldY};
    }

    std::shared_ptr<Node> hitTest(double screenX, double screenY, int width, int height) {
        auto [worldX, worldY] = screenToWorld(screenX, screenY, width, height);
        return map->hitTest(worldX, worldY);
    }
};

#endif // DRAWING_CONTEXT_HPP