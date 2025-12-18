#include "MapArea.hpp"
#include "Constants.hpp"
#include "MindMap.hpp"
#include "MindMapDrawer.hpp"
#include <gdk/gdkkeysyms.h>
#include <cmath>
#include <algorithm>

MapArea::MapArea(std::shared_ptr<MindMap> m) : drawingContext(m) {
    add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
               Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
    drawingContext.setRedrawCallback([this](){ this->queue_draw(); });
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

bool MapArea::on_button_press_event(GdkEventButton* event) {
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    // Handle node selection and dragging
    auto clickedNode = drawingContext.hitTest(event->x, event->y, width, height);

    // Handle Right Click (Context Menu)
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        if (clickedNode) {
            // Select the node user clicked on (good UX)
            drawingContext.setSelectedNode(clickedNode);
            queue_draw();
            
            // Emit signal for main window to show menu
            signal_node_context_menu.emit(event, clickedNode);
            return true; // Stop propagation
        }
    }

    // Check if Ctrl is pressed for panning - BUT only if not clicking on a node
    if ((event->state & GDK_CONTROL_MASK) && !clickedNode) {
        return handlePanningStart(event);
    }
    
    if (event->type == GDK_2BUTTON_PRESS && clickedNode) {
        drawingContext.setSelectedNode(clickedNode);
        signal_edit_node.emit(clickedNode);
        queue_draw();
        return true;
    }
    
    if (clickedNode) {
        return handleNodeSelection(event, clickedNode);
    } else {
        // Clicked on empty space - clear selection
        bool isCtrlPressed = (event->state & GDK_CONTROL_MASK) != 0;
        if (!isCtrlPressed) {
            drawingContext.clearSelection();
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
        // Regular click - if the clicked node is already selected AND there are multiple selections,
        // drag all selected nodes; otherwise, select only this node
        bool isAlreadySelected = drawingContext.isNodeSelected(clickedNode);
        bool hasMultipleSelection = drawingContext.getSelectedNodesCount() > 1;

        if (isAlreadySelected && hasMultipleSelection) {
            // The clicked node is part of a multi-selection, prepare to drag all selected nodes
            // Set it as the primary selected node to bring it to front if needed
            drawingContext.setSelectedNode(clickedNode);  // This ensures clicked node becomes primary selection

            // Prepare for potential dragging with threshold
            isPreDragging = true;  // Indicate potential drag, will confirm on motion
            isDragging = false;    // Actual dragging hasn't started yet
            isFirstDragMotion = true;  // Reset for this potential drag operation
            dragStartX = event->x;
            dragStartY = event->y;
            // Store original position of the clicked node to calculate movement
            nodeStartX = clickedNode->x;
            nodeStartY = clickedNode->y;
        } else {
            // Select only this node - this handles single clicks properly
            drawingContext.setSelectedNode(clickedNode);

            // Prepare for potential dragging with threshold
            isPreDragging = true;  // Indicate potential drag, will confirm on motion
            isDragging = false;    // Actual dragging hasn't started yet
            isFirstDragMotion = true;  // Reset for this potential drag operation
            dragStartX = event->x;
            dragStartY = event->y;
            // Store original node position in world coordinates
            nodeStartX = clickedNode->x;
            nodeStartY = clickedNode->y;
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
    // If we were in pre-drag state but didn't exceed threshold, node is selected but not dragged
    // If we were in actual dragging state, dragging stops
    isDragging = false;
    isPanning = false;
    isPreDragging = false;
    isFirstDragMotion = true;  // Reset for next drag operation
    return true;
}

bool MapArea::on_motion_notify_event(GdkEventMotion* event) {
    if (isPanning) {
        return handlePanningMove(event);
    } else if (isPreDragging && !isDragging) {
        // Check if mouse has moved beyond drag threshold to start actual dragging
        const double DRAG_THRESHOLD = 3.0; // Threshold in pixels to start dragging
        double distance = sqrt(pow(event->x - dragStartX, 2) + pow(event->y - dragStartY, 2));

        if (distance >= DRAG_THRESHOLD) {
            // Mouse has moved beyond threshold, start actual dragging
            isDragging = true;
            isFirstDragMotion = true;  // Reset for this drag operation
            // No need to re-select here as the node is already selected from button press
        } else {
            // Mouse hasn't moved enough, still in pre-drag state
            return true;
        }
    } else if (isDragging) {
        return handleNodeDragMove(event);
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

    // Move all selected nodes by the same delta
    for (auto& node : selectedNodes) {
        if (node) {
            // Apply the incremental offset to the current node position
            node->x += deltaX;
            node->y += deltaY;
            node->manualPosition = true;

            // Move the entire subtree by the same incremental offset
            moveSubtree(node, deltaX, deltaY);
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

void MapArea::invalidateLayout() {
    drawingContext.invalidateLayout();
    queue_draw();
}
