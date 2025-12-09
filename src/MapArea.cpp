#include "MapArea.hpp"
#include "Constants.hpp"

bool MapArea::on_button_press_event(GdkEventButton* event) {
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    // Handle node selection and dragging
    auto clickedNode = drawingContext.hitTest(event->x, event->y, width, height);

    // Check if Ctrl is pressed for panning - BUT only if not clicking on a node
    if ((event->state & GDK_CONTROL_MASK) && !clickedNode) {
        isPanning = true;
        panStartOffsetX = drawingContext.getViewport().offsetX;
        panStartOffsetY = drawingContext.getViewport().offsetY;
        dragStartX = event->x;
        dragStartY = event->y;
        return true;
    }
    if (event->type == GDK_2BUTTON_PRESS && clickedNode) {
        drawingContext.setSelectedNode(clickedNode);
        signal_edit_node.emit(clickedNode);
        queue_draw();
        return true;
    }
    if (clickedNode) {
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
        } else {
            // Regular click - if the clicked node is already selected AND there are multiple selections,
            // drag all selected nodes; otherwise, select only this node
            bool isAlreadySelected = drawingContext.isNodeSelected(clickedNode);
            bool hasMultipleSelection = drawingContext.getSelectedNodesCount() > 1;

            if (isAlreadySelected && hasMultipleSelection) {
                // The clicked node is part of a multi-selection, drag all selected nodes
                isDragging = true;
                isFirstDragMotion = true;  // Reset for this drag operation
                dragStartX = event->x;
                dragStartY = event->y;
                // Store original position of the clicked node to calculate movement
                auto [worldX, worldY] = drawingContext.screenToWorld(event->x, event->y, width, height);
                nodeStartX = clickedNode->x;
                nodeStartY = clickedNode->y;
            } else {
                // Select only this node and drag it
                drawingContext.setSelectedNode(clickedNode);
                isDragging = true;
                isFirstDragMotion = true;  // Reset for this drag operation
                dragStartX = event->x;
                dragStartY = event->y;
                // Store original node position in world coordinates
                auto [worldX, worldY] = drawingContext.screenToWorld(event->x, event->y, width, height);
                nodeStartX = clickedNode->x;
                nodeStartY = clickedNode->y;
            }
        }
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

bool MapArea::on_button_release_event(GdkEventButton* event) {
    isDragging = false;
    isPanning = false;
    isFirstDragMotion = true;  // Reset for next drag operation
    return true;
}

bool MapArea::on_motion_notify_event(GdkEventMotion* event) {
    if (isPanning) {
        // Panning: update viewport offset
        double dx = event->x - dragStartX;
        double dy = event->y - dragStartY;
        Viewport vp = drawingContext.getViewport();
        vp.offsetX = panStartOffsetX + dx;
        vp.offsetY = panStartOffsetY + dy;
        drawingContext.setViewport(vp);
        queue_draw();
        return true;
    } else if (isDragging) {
        // Check which dragging mode we're in: single node or multiple nodes
        auto selectedNodes = drawingContext.getSelectedNodes();
        if (!selectedNodes.empty()) {
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
    }
    return false;
}

bool MapArea::on_scroll_event(GdkEventScroll* event) {
    // Handle zoom with scroll wheel (reduce zoom factor to make it much less aggressive)
    double zoomFactor = (event->direction == GDK_SCROLL_UP) ? 1.05 : 1.0/1.05;

    Viewport vp = drawingContext.getViewport();

    // Calculate the zoom factor based on scroll direction
    double newScale = vp.scale * zoomFactor;
    // Limit zoom to reasonable levels
    newScale = std::max(E4Maps::MIN_ZOOM, std::min(E4Maps::MAX_ZOOM, newScale));

    // Calculate mouse position relative to center of the view
    Gtk::Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    // Calculate the world coordinates of the point currently under the mouse cursor
    // In the current coordinate system:
    // ScreenX = width/2 + vp.offsetX + worldX * vp.scale
    // So: worldX = (ScreenX - width/2 - vp.offsetX) / vp.scale
    double worldMouseX = (event->x - width/2.0 - vp.offsetX) / vp.scale;
    double worldMouseY = (event->y - height/2.0 - vp.offsetY) / vp.scale;

    // Update the scale
    vp.scale = newScale;

    // Adjust offsets to keep the same world point under the mouse cursor
    // ScreenX = width/2 + newOffsetX + worldX * newScale
    // So: newOffsetX = ScreenX - width/2 - worldX * newScale
    vp.offsetX = event->x - width/2.0 - worldMouseX * vp.scale;
    vp.offsetY = event->y - height/2.0 - worldMouseY * vp.scale;

    drawingContext.setViewport(vp);
    queue_draw();
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

void MapArea::zoomIn() {
    Viewport vp = drawingContext.getViewport();
    vp.scale *= E4Maps::ZOOM_FACTOR_IN;  // Reduced from 1.2 to make zoom less aggressive
    vp.scale = std::min(E4Maps::MAX_ZOOM, vp.scale);  // Limit maximum zoom
    drawingContext.setViewport(vp);
    queue_draw();
}

void MapArea::zoomOut() {
    Viewport vp = drawingContext.getViewport();
    vp.scale *= E4Maps::ZOOM_FACTOR_OUT;  // Reduced from 1.0/1.2 to make zoom less aggressive
    vp.scale = std::max(E4Maps::MIN_ZOOM, vp.scale);  // Limit minimum zoom
    drawingContext.setViewport(vp);
    queue_draw();
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