#include "MapArea.hpp"
#include "Constants.hpp"
#include "MindMap.hpp"
#include "MindMapDrawer.hpp"
#include "ImageCache.hpp"
#include <gdk/gdkkeysyms.h>
#include <cmath>
#include <algorithm>

MapArea::MapArea(std::shared_ptr<MindMap> m) : drawingContext(m) {
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
               Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK | Gdk::KEY_PRESS_MASK);
    drawingContext.setRedrawCallback([this](){ this->queue_draw(); });
    set_can_focus(true); // Enable focus for keyboard events
}

void MapArea::setMap(std::shared_ptr<MindMap> m) {
    drawingContext.setMap(m);
    ImageCache::getInstance().clear();
    // Center the view to show all content
    Gtk::Allocation allocation = get_allocation();
    drawingContext.centerView(allocation.get_width(), allocation.get_height());
    queue_draw();
}

void MapArea::setSelectedNodes(const std::vector<std::shared_ptr<Node>>& nodes) {
    drawingContext.setSelectedNodes(nodes);
    queue_draw();
}

bool MapArea::on_key_press_event(GdkEventKey* event) {
    // Handle Navigation
    if (event->keyval == GDK_KEY_Left) {
        navigateSelection(-1, 0);
        return true;
    } else if (event->keyval == GDK_KEY_Right) {
        navigateSelection(1, 0);
        return true;
    } else if (event->keyval == GDK_KEY_Up) {
        navigateSelection(0, -1);
        return true;
    } else if (event->keyval == GDK_KEY_Down) {
        navigateSelection(0, 1);
        return true;
    } 
    
    // Handle shortcuts for adding nodes
    if (event->keyval == GDK_KEY_Insert || event->keyval == GDK_KEY_Tab) {
        signal_add_child_node.emit();
        return true;
    } else if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
        // Shift+Enter could mean "add parent" or something else, but standard Enter is sibling
        signal_add_sibling_node.emit();
        return true;
    }

    return Gtk::DrawingArea::on_key_press_event(event);
}

void MapArea::navigateSelection(int directionX, int directionY) {
    auto selectedNode = getSelectedNode();
    if (!selectedNode) {
        // If nothing selected, select root
        auto map = drawingContext.getMap();
        if (map && map->root) {
            std::vector<std::shared_ptr<Node>> nodes = {map->root};
            setSelectedNodes(nodes);
            centerViewOnNode(map->root);
        }
        return;
    }

    // Find the best candidate node in the requested direction
    // Algorithm:
    // 1. Filter nodes that are in the correct general direction relative to current node
    // 2. Score them based on distance and angle
    
    std::shared_ptr<Node> bestCandidate = nullptr;
    double bestScore = std::numeric_limits<double>::max();

    auto map = drawingContext.getMap();
    if (!map || !map->root) return;

    // Collect all nodes
    std::vector<std::shared_ptr<Node>> allNodes;
    std::function<void(std::shared_ptr<Node>)> collect = [&](std::shared_ptr<Node> n) {
        allNodes.push_back(n);
        for(auto& c : n->children) collect(c);
    };
    collect(map->root);

    for (const auto& node : allNodes) {
        if (node == selectedNode) continue;

        double dx = node->x - selectedNode->x;
        double dy = node->y - selectedNode->y;

        // Check direction
        bool correctDir = false;
        if (directionX > 0) correctDir = (dx > 0);       // Right
        else if (directionX < 0) correctDir = (dx < 0);  // Left
        else if (directionY > 0) correctDir = (dy > 0);  // Down
        else if (directionY < 0) correctDir = (dy < 0);  // Up
        
        if (!correctDir) continue;
        
        // Refinement: Ideally we want something primarily in that direction.
        // E.g. for Right, we prefer small dy. 
        
        // Score calculation:
        // Dist^2 + Penalty for perpendicular distance
        double distSq = dx*dx + dy*dy;
        
        // Perpendicular component importance
        double perpDistSq = 0;
        if (directionX != 0) perpDistSq = dy*dy;
        else perpDistSq = dx*dx;

        // We weight perpendicular distance heavily to avoid jumping to a far-away node 
        // that is technically "to the right" but visually unrelated.
        double score = distSq + 5.0 * perpDistSq;

        if (score < bestScore) {
            bestScore = score;
            bestCandidate = node;
        }
    }

    if (bestCandidate) {
        setSelectedNodes({bestCandidate});
        centerViewOnNode(bestCandidate);
    }
}

bool MapArea::on_button_press_event(GdkEventButton* event) {
    // Ensure we grab focus on click so keyboard events work
    if (!has_focus()) grab_focus();
    
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    // Handle node selection and dragging
    auto clickedNode = drawingContext.hitTest(event->x, event->y, width, height);

    // If no node clicked, check for connections
    std::shared_ptr<Connection> clickedConnection = nullptr;
    if (!clickedNode) {
        clickedConnection = drawingContext.hitTestConnection(event->x, event->y, width, height);
    }

    // Handle Right Click (Context Menu)
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        if (clickedNode) {
            // Select the node user clicked on (good UX)
            drawingContext.setSelectedNode(clickedNode);
            queue_draw();
            
            // Emit signal for main window to show menu
            signal_node_context_menu.emit(event, clickedNode);
            return true; // Stop propagation
        } else if (clickedConnection) {
            // Select connection
            drawingContext.setSelectedConnection(clickedConnection);
            queue_draw();
            
            // Emit signal for main window to show connection context menu
            signal_connection_context_menu.emit(event, clickedConnection);
            return true;
        }
    }

    // Check if Ctrl is pressed for panning - BUT only if not clicking on a node
    if ((event->state & GDK_CONTROL_MASK) && !clickedNode && !clickedConnection) {
        return handlePanningStart(event);
    }
    
    if (event->type == GDK_2BUTTON_PRESS && clickedNode) {
        drawingContext.setSelectedNode(clickedNode);

        // Reset all drag-related state since the modal dialog will consume
        // the button release event.
        isDragging = false;
        isPreDragging = false;
        isPanning = false;
        isFirstDragMotion = true;
        initialDragNodes.clear();
        initialDragPositions.clear();

        signal_edit_node.emit(clickedNode);
        queue_draw();
        return true;
    }
    
    if (clickedNode) {
        return handleNodeSelection(event, clickedNode);
    } else if (clickedConnection) {
        // Handle connection selection
        bool isCtrlPressed = (event->state & GDK_CONTROL_MASK) != 0;
        if (!isCtrlPressed) {
            drawingContext.setSelectedConnection(clickedConnection);
        } else {
             // Ctrl + click on connection currently acts as regular selection
             // (Multi-selection of connections not yet supported)
             drawingContext.setSelectedConnection(clickedConnection);
        }
        isDragging = false;
    } else {
        // Clicked on empty space - clear selection
        bool isCtrlPressed = (event->state & GDK_CONTROL_MASK) != 0;
        if (!isCtrlPressed) {
            drawingContext.clearAllSelection();
        }
        isDragging = false;
    }
    queue_draw();
    return true;
}

bool MapArea::handleNodeSelection(GdkEventButton* event, std::shared_ptr<Node> clickedNode) {
    if (!clickedNode) return false;

    // Check if Ctrl is pressed to toggle selection
    bool isCtrlPressed = (event->state & GDK_CONTROL_MASK) != 0;

    if (isCtrlPressed) {
        // Toggle the clicked node in the selection
        bool wasSelected = drawingContext.isNodeSelected(clickedNode);

        if (wasSelected) {
            drawingContext.removeNodeFromSelection(clickedNode);
        } else {
            drawingContext.addNodeToSelection(clickedNode);
        }

        // We're not dragging in this case, just selecting/deselecting
        isDragging = false;
        isPreDragging = false;
    } else {
        // Regular click - if the clicked node is already selected, don't clear others
        // to allow dragging the whole group.
        bool isAlreadySelected = drawingContext.isNodeSelected(clickedNode);

        if (!isAlreadySelected) {
            // Select only this node
            drawingContext.setSelectedNodes({clickedNode});
        } else {
            // If already selected, just make it the primary selected node WITHOUT clearing the list
            drawingContext.setSelectedNodeWithoutClearing(clickedNode);
        }

        // Prepare for potential dragging with threshold
        isPreDragging = true;  // Indicate potential drag, will confirm on motion
        isDragging = false;    // Actual dragging hasn't started yet
        isFirstDragMotion = true;  // Reset for this potential drag operation
        dragStartX = event->x;
        dragStartY = event->y;
        // Store original node position in world coordinates
        nodeStartX = clickedNode->x;
        nodeStartY = clickedNode->y;

        // Store initial positions of all selected nodes for Undo support
        initialDragNodes = drawingContext.getSelectedNodes();
        initialDragPositions.clear();
        for (const auto& node : initialDragNodes) {
            initialDragPositions.push_back({node->x, node->y});
        }
    }

    // Queue redraw to update visual representation of selection immediately
    queue_draw();
    return true;
}

bool MapArea::handlePanningStart(GdkEventButton* event) {
    isPanning = true;
    panStartOffsetX = drawingContext.getViewport().offsetX;
    panStartOffsetY = drawingContext.getViewport().offsetY;
    dragStartX = event->x;
    dragStartY = event->y;
    return true;
}

bool MapArea::on_button_release_event(GdkEventButton* event) {
    if (isDragging) {
        // Dragging finished, collect final positions and emit signal
        std::vector<std::pair<double, double>> finalPositions;
        for (const auto& node : initialDragNodes) {
            finalPositions.push_back({node->x, node->y});
        }
        
        // Only emit if there was an actual movement
        bool moved = false;
        for (size_t i = 0; i < finalPositions.size(); ++i) {
            if (std::abs(finalPositions[i].first - initialDragPositions[i].first) > 0.1 ||
                std::abs(finalPositions[i].second - initialDragPositions[i].second) > 0.1) {
                moved = true;
                break;
            }
        }
        
        if (moved) {
            signal_nodes_moved.emit(initialDragNodes, initialDragPositions, finalPositions);
        }
    }

    // If we were in pre-drag state but didn't exceed threshold, node is selected but not dragged
    // If we were in actual dragging state, dragging stops
    isDragging = false;
    isPanning = false;
    isPreDragging = false;
    isFirstDragMotion = true;  // Reset for next drag operation
    initialDragNodes.clear();
    initialDragPositions.clear();
    return true;
}

bool MapArea::on_motion_notify_event(GdkEventMotion* event) {
    if (isPanning) {
        return handlePanningMove(event);
    } else if (isPreDragging && !isDragging) {
        // ... (existing drag logic) ...
        const double DRAG_THRESHOLD = 3.0;
        double distance = sqrt(pow(event->x - dragStartX, 2) + pow(event->y - dragStartY, 2));

        if (distance >= DRAG_THRESHOLD) {
            isDragging = true;
            isFirstDragMotion = true;
        } else {
            return true;
        }
    } else if (isDragging) {
        return handleNodeDragMove(event);
    } else {
        // Not dragging or panning: handle Hover effects
        Gtk::Allocation allocation = get_allocation();
        auto hovered = drawingContext.hitTestConnection(event->x, event->y, allocation.get_width(), allocation.get_height());
        
        auto currentHovered = drawingContext.getHoveredConnection();
        if (hovered != currentHovered) {
            drawingContext.setHoveredConnection(hovered);
            
            // Change cursor to hand if hovering over a connection
            auto window = get_window();
            if (window) {
                if (hovered) {
                    window->set_cursor(Gdk::Cursor::create(Gdk::HAND2));
                } else {
                    window->set_cursor(); // Reset to default
                }
            }
            queue_draw();
        }
    }
    return false;
}

bool MapArea::handlePanningMove(GdkEventMotion* event) {
    double dx = event->x - dragStartX;
    double dy = event->y - dragStartY;
    Viewport vp = drawingContext.getViewport();
    vp.offsetX = panStartOffsetX + dx;
    vp.offsetY = panStartOffsetY + dy;
    drawingContext.setViewport(vp);
    queue_draw();
    return true;
}

bool MapArea::handleNodeDragMove(GdkEventMotion* event) {
    // Check which dragging mode we're in: single node or multiple nodes
    auto selectedNodes = drawingContext.getSelectedNodes();
    if (selectedNodes.empty()) return false;

    // Dragging nodes: update position incrementally
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    auto [worldCurrentX, worldCurrentY] = drawingContext.screenToWorld(event->x, event->y, width, height);

    // Calculate incremental offset from previous position
    double deltaX, deltaY;
    if (isFirstDragMotion) {
        // On first motion event, use absolute position to avoid jumps
        // Calculate how much the mouse has moved from the start position
        auto [worldStartX, worldStartY] = drawingContext.screenToWorld(dragStartX, dragStartY, width, height);
        deltaX = worldCurrentX - worldStartX;
        deltaY = worldCurrentY - worldStartY;

        // Set the reference points for incremental movement
        prevMouseWorldX = worldCurrentX;
        prevMouseWorldY = worldCurrentY;
        isFirstDragMotion = false;
    } else {
        // Calculate incremental movement since last event
        deltaX = worldCurrentX - prevMouseWorldX;
        deltaY = worldCurrentY - prevMouseWorldY;

        // Update the reference points for next iteration
        prevMouseWorldX = worldCurrentX;
        prevMouseWorldY = worldCurrentY;
    }

    // Move all selected nodes by the same delta, BUT only if their parent isn't also selected
    // to avoid moving subtrees multiple times.
    for (auto& node : selectedNodes) {
        if (node) {
            bool parentIsSelected = false;
            if (auto p = node->parent.lock()) {
                parentIsSelected = drawingContext.isNodeSelected(p);
            }

            if (!parentIsSelected) {
                // Apply the incremental offset to the current node position
                node->x += deltaX;
                node->y += deltaY;
                node->manualPosition = true;

                // Move the entire subtree by the same incremental offset
                moveSubtree(node, deltaX, deltaY);
            }
        }
    }

    queue_draw();
    // Signal that the map has been modified
    signal_map_modified.emit();
    return true;
}

bool MapArea::on_scroll_event(GdkEventScroll* event) {
    // Handle zoom with scroll wheel (reduce zoom factor to make it much less aggressive)
    double zoomFactor = (event->direction == GDK_SCROLL_UP) ? 1.05 : 1.0/1.05;
    
    zoomAtPoint(zoomFactor, event->x, event->y);
    return true;
}

bool MapArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    return drawingContext.on_draw(cr, width, height);
}

bool MapArea::on_configure_event(GdkEventConfigure* event) {
    // Re-center the view when the window is resized
    drawingContext.centerView(event->width, event->height);
    return Gtk::DrawingArea::on_configure_event(event);
}

void MapArea::zoomAtPoint(double factor, double screenX, double screenY) {
    Viewport vp = drawingContext.getViewport();
    
    // Calculate new scale
    double newScale = vp.scale * factor;
    // Limit zoom to reasonable levels
    newScale = std::max(E4Maps::MIN_ZOOM, std::min(E4Maps::MAX_ZOOM, newScale));
    
    // If scale didn't change (hit limits), do nothing
    if (newScale == vp.scale) return;

    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    // Calculate world coordinates of the point under mouse/center
    double worldX = (screenX - width/2.0 - vp.offsetX) / vp.scale;
    double worldY = (screenY - height/2.0 - vp.offsetY) / vp.scale;

    // Update scale
    vp.scale = newScale;

    // Adjust offsets to keep the same world point under the screen point
    vp.offsetX = screenX - width/2.0 - worldX * vp.scale;
    vp.offsetY = screenY - height/2.0 - worldY * vp.scale;

    drawingContext.setViewport(vp);
    queue_draw();
}

void MapArea::zoomIn() {
    Gtk::Allocation allocation = get_allocation();
    zoomAtPoint(E4Maps::ZOOM_FACTOR_IN, allocation.get_width() / 2.0, allocation.get_height() / 2.0);
}

void MapArea::zoomOut() {
    Gtk::Allocation allocation = get_allocation();
    zoomAtPoint(E4Maps::ZOOM_FACTOR_OUT, allocation.get_width() / 2.0, allocation.get_height() / 2.0);
}

void MapArea::resetView() {
    Gtk::Allocation allocation = get_allocation();
    drawingContext.resetViewToCenter(allocation.get_width(), allocation.get_height());
    queue_draw();
}

void MapArea::moveSubtree(std::shared_ptr<Node> node, double dx, double dy) {
    if (!node) return;

    // Move all children of this node
    for (auto& child : node->children) {
        // Only move nodes that are manually positioned (or if we want to move all)
        // This is important to maintain auto-layout behavior for non-manual nodes
        child->x += dx;
        child->y += dy;
        child->manualPosition = true;  // Mark as manually positioned

        // Recursively move the child's subtree
        moveSubtree(child, dx, dy);
    }
}

bool MapArea::getNodeScreenRect(std::shared_ptr<Node> node, Gdk::Rectangle& rect) {
    if (!node) return false;
    
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
    
    const Viewport& vp = drawingContext.getViewport();
    
    // Calculate center of node in screen coordinates
    double screenX = width / 2.0 + vp.offsetX + node->x * vp.scale;
    double screenY = height / 2.0 + vp.offsetY + node->y * vp.scale;
    
    // Calculate dimensions in screen coordinates
    double screenW = node->width * vp.scale;
    double screenH = node->height * vp.scale;
    
    rect.set_x((int)(screenX - screenW / 2.0));
    rect.set_y((int)(screenY - screenH / 2.0));
    rect.set_width((int)std::ceil(screenW));
    rect.set_height((int)std::ceil(screenH));
    
    return true;
}

void MapArea::centerViewOnNode(std::shared_ptr<Node> node) {
    if (!node) return;

    Viewport vp = drawingContext.getViewport();
    
    // Calculate offsets to center the node
    // Formula derived from: screenX = width/2 + vp.offsetX + worldX * vp.scale
    // We want screenX = width/2, so 0 = vp.offsetX + worldX * vp.scale
    vp.offsetX = -node->x * vp.scale;
    vp.offsetY = -node->y * vp.scale;

    drawingContext.setViewport(vp);
    queue_draw();
}

void MapArea::invalidateLayout() {
    drawingContext.invalidateLayout();
    queue_draw();
}
