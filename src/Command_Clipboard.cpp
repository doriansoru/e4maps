#include "Command.hpp"
#include "Translation.hpp"
#include <cmath>
#include <algorithm>
#include <map>
#include <functional>

// CopyNodeCommand
CopyNodeCommand::CopyNodeCommand(std::shared_ptr<Node> node)
    : nodeToCopy(node), executed(false) {}

void CopyNodeCommand::execute() {
    if (!executed && nodeToCopy) {
        nodeCopy = cloneNodeTree(nodeToCopy);
        executed = true;
    }
}

void CopyNodeCommand::undo() {
    // Copy operation doesn't modify the map, so undo is a no-op
    if (executed) {
        nodeCopy.reset();
        executed = false;
    }
}

std::string CopyNodeCommand::getName() const {
    return _("Copy Node");
}

std::shared_ptr<Node> CopyNodeCommand::getNodeCopy() const {
    return nodeCopy;
}

// CutNodeCommand
CutNodeCommand::CutNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToCut)
    : parent(parentNode), nodeToCut(nodeToCut), executed(false) {}

void CutNodeCommand::execute() {
    if (!executed && parent && nodeToCut) {
        // Find the position of the node in parent's children
        auto it = std::find(parent->children.begin(), parent->children.end(), nodeToCut);
        if (it != parent->children.end()) {
            position = std::distance(parent->children.begin(), it);
            nodeCopy = cloneNodeTree(nodeToCut);  // Keep a copy before removal
            parent->removeChild(nodeToCut);
            executed = true;
        }
    }
}

void CutNodeCommand::undo() {
    if (executed && parent && nodeCopy) {
        // Reinsert the node copy at its original position
        if (position < parent->children.size()) {
            parent->children.insert(parent->children.begin() + position, cloneNodeTree(nodeCopy));
        } else {
            parent->addChild(cloneNodeTree(nodeCopy));
        }
        executed = false;
    }
}

std::string CutNodeCommand::getName() const {
    return _("Cut Node");
}

std::shared_ptr<Node> CutNodeCommand::getNodeCopy() const {
    return nodeCopy;
}

// PasteNodeCommand
PasteNodeCommand::PasteNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToPaste)
    : parent(parentNode), nodeToPaste(nodeToPaste), executed(false) {}

void PasteNodeCommand::execute() {
    if (!executed && parent && nodeToPaste) {
        actualPastedNode = cloneNodeTree(nodeToPaste);
        if (actualPastedNode) {
            // Apply intelligent positioning to avoid overlapping with existing children
            if (actualPastedNode->manualPosition) {
                // Calculate a suitable position that doesn't overlap with existing children
                std::pair<double, double> newPos = findNonOverlappingPosition(parent, actualPastedNode);
                actualPastedNode->x = newPos.first;
                actualPastedNode->y = newPos.second;
            }

            parent->addChild(actualPastedNode);
        }
        executed = true;
    }
}

void PasteNodeCommand::undo() {
    if (executed && parent && actualPastedNode) {
        parent->removeChild(actualPastedNode);
        executed = false;
    }
}

std::string PasteNodeCommand::getName() const {
    return _("Paste Node");
}

std::pair<double, double> PasteNodeCommand::findNonOverlappingPosition(std::shared_ptr<Node> targetParent, std::shared_ptr<Node> nodeToPaste) {
    // Calculate the offset that should be applied to the entire subtree
    double rootOriginalX = nodeToPaste->x;
    double rootOriginalY = nodeToPaste->y;

    // Start with a basic offset from the original root position
    double newRootX = rootOriginalX + 40.0;
    double newRootY = rootOriginalY + 40.0;

    // Get a list of all children positions of the target parent to check for overlaps
    std::vector<std::pair<double, double>> existingPositions;
    for (const auto& child : targetParent->children) {
        existingPositions.push_back({child->x, child->y});
    }

    // Keep trying different positions until we find one that doesn't overlap
    int attempts = 0;
    const int maxAttempts = 100; // Prevent infinite loops

    while (attempts < maxAttempts) {
        bool overlaps = false;

        // Check if the new root position overlaps with any existing child
        for (const auto& pos : existingPositions) {
            // Simple distance check - if closer than a threshold, consider it overlapping
            double distance = std::sqrt(std::pow(newRootX - pos.first, 2) + std::pow(newRootY - pos.second, 2));
            if (distance < 60.0) { // 60 pixel minimum distance
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            break; // Found a good position
        }

        // Try a different position using a spiral pattern
        attempts++;
        double angle = attempts * 0.785; // Approximately 45 degrees in radians
        double radius = (attempts / 8) * 60.0; // Increase radius every 8 attempts
        newRootX = targetParent->x + radius * std::cos(angle);
        newRootY = targetParent->y + radius * std::sin(angle);
    }

    // If we've exhausted attempts, use a fallback position
    if (attempts >= maxAttempts) {
        newRootX = targetParent->x + 100.0;
        newRootY = targetParent->y + 100.0;
    }

    // Now apply the calculated offset to the entire subtree
    double offsetX = newRootX - rootOriginalX;
    double offsetY = newRootY - rootOriginalY;

    // Apply the offset to the root node
    nodeToPaste->x = newRootX;
    nodeToPaste->y = newRootY;

    // Apply the same offset to all descendants in the subtree
    applyOffsetToSubtree(nodeToPaste, offsetX, offsetY);

    // Return the new root position
    return std::make_pair(newRootX, newRootY);
}

void PasteNodeCommand::applyOffsetToSubtree(std::shared_ptr<Node> node, double offsetX, double offsetY) {
    if (!node) return;

    for (auto& child : node->children) {
        child->x += offsetX;
        child->y += offsetY;

        // Recursively apply offset to grandchildren
        applyOffsetToSubtree(child, offsetX, offsetY);
    }
}

// CopyMultipleNodesCommand
CopyMultipleNodesCommand::CopyMultipleNodesCommand(std::shared_ptr<MindMap> m, const std::vector<std::shared_ptr<Node>>& nodes)
    : map(m), nodesToCopy(nodes), executed(false) {}

void CopyMultipleNodesCommand::execute() {
    if (!executed) {
        nodesCopy.clear();
        connectionsCopy.clear();
        
        // Map original nodes to their copies
        std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> originalToCopy;
        
        // Helper to populate map recursively
        std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> mapNodes = 
            [&](std::shared_ptr<Node> original, std::shared_ptr<Node> copy) {
                originalToCopy[original] = copy;
                if (original->children.size() == copy->children.size()) {
                    for (size_t i = 0; i < original->children.size(); ++i) {
                        mapNodes(original->children[i], copy->children[i]);
                    }
                }
            };

        for (auto& node : nodesToCopy) {
            if (node) {
                auto copy = cloneNodeTree(node);
                nodesCopy.push_back(copy);
                mapNodes(node, copy);
            }
        }
        
        // Find connections entirely within the copied set
        if (map) {
            for (const auto& conn : map->connections) {
                auto itFrom = originalToCopy.find(conn->from);
                auto itTo = originalToCopy.find(conn->to);
                
                if (itFrom != originalToCopy.end() && itTo != originalToCopy.end()) {
                    // This connection is internal to the copied selection
                    auto newConn = std::make_shared<Connection>(*conn);
                    newConn->from = itFrom->second;
                    newConn->to = itTo->second;
                    connectionsCopy.push_back(newConn);
                }
            }
        }
        
        executed = true;
    }
}

void CopyMultipleNodesCommand::undo() {
    // Copy operation doesn't modify the map, so undo is a no-op
    if (executed) {
        nodesCopy.clear();
        connectionsCopy.clear();
        executed = false;
    }
}

std::string CopyMultipleNodesCommand::getName() const {
    return _("Copy Multiple Nodes");
}

const std::vector<std::shared_ptr<Node>>& CopyMultipleNodesCommand::getNodesCopy() const {
    return nodesCopy;
}

const std::vector<std::shared_ptr<Connection>>& CopyMultipleNodesCommand::getConnectionsCopy() const {
    return connectionsCopy;
}

// CutMultipleNodesCommand
CutMultipleNodesCommand::CutMultipleNodesCommand(std::shared_ptr<MindMap> m, const std::vector<std::shared_ptr<Node>>& nodes) 
    : map(m), executed(false) {
    for (auto& node : nodes) {
        if (node && !node->isRoot()) {  // Can't cut the root node
            if (auto parent = node->parent.lock()) {
                // Find the position of the node in parent's children
                auto it = std::find(parent->children.begin(), parent->children.end(), node);
                if (it != parent->children.end()) {
                    std::size_t position = std::distance(parent->children.begin(), it);
                    parentChildPairs.push_back({parent, node});
                    positions.push_back(position);
                }
            }
        }
    }
}

void CutMultipleNodesCommand::execute() {
    if (!executed) {
        nodesCopy.clear();
        connectionsCopy.clear();
        removedConnections.clear();

        // Map original nodes to their copies (for clipboard)
        std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> originalToCopy;
        
        // Helper to populate map recursively
        std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> mapNodes = 
            [&](std::shared_ptr<Node> original, std::shared_ptr<Node> copy) {
                originalToCopy[original] = copy;
                if (original->children.size() == copy->children.size()) {
                    for (size_t i = 0; i < original->children.size(); ++i) {
                        mapNodes(original->children[i], copy->children[i]);
                    }
                }
            };
            
        // Collect all nodes being cut (including descendants) to identify affected connections
        std::vector<std::shared_ptr<Node>> allCutNodes;
        std::function<void(std::shared_ptr<Node>)> collectNodes = 
            [&](std::shared_ptr<Node> node) {
                allCutNodes.push_back(node);
                for (auto& child : node->children) collectNodes(child);
            };

        // Keep copies of nodes before removal and map them
        for (auto& pair : parentChildPairs) {
            auto node = pair.second;
            auto copy = cloneNodeTree(node);
            nodesCopy.push_back(copy);
            mapNodes(node, copy);
            collectNodes(node);
        }

        // Handle Connections
        if (map) {
            std::vector<std::shared_ptr<Connection>> remainingConnections;
            
            for (const auto& conn : map->connections) {
                bool isInternal = false;
                bool isAffected = false;
                
                auto itFrom = originalToCopy.find(conn->from);
                auto itTo = originalToCopy.find(conn->to);
                
                if (itFrom != originalToCopy.end() && itTo != originalToCopy.end()) {
                    isInternal = true;
                }
                
                // Check if connection involves any cut node
                for (const auto& cutNode : allCutNodes) {
                    if (conn->from == cutNode || conn->to == cutNode) {
                        isAffected = true;
                        break;
                    }
                }
                
                if (isInternal) {
                    // Copy internal connections to clipboard
                    auto newConn = std::make_shared<Connection>(*conn);
                    newConn->from = itFrom->second;
                    newConn->to = itTo->second;
                    connectionsCopy.push_back(newConn);
                }
                
                if (isAffected) {
                    // Remove from map (store for undo)
                    removedConnections.push_back(conn);
                } else {
                    remainingConnections.push_back(conn);
                }
            }
            
            map->connections = remainingConnections;
        }

        // Remove nodes from their parents
        for (size_t i = 0; i < parentChildPairs.size(); i++) {
            auto& pair = parentChildPairs[i];
            if (pair.first && pair.second) {
                pair.first->removeChild(pair.second);
            }
        }
        executed = true;
    }
}

void CutMultipleNodesCommand::undo() {
    if (executed) {
        // Reinsert nodes at their original positions
        for (size_t i = 0; i < parentChildPairs.size(); i++) {
            auto& pair = parentChildPairs[i];
            if (pair.first && !nodesCopy.empty() && i < nodesCopy.size()) {
                auto originalNode = pair.second;
                
                // Reinsert at original position
                if (positions[i] < pair.first->children.size()) {
                    pair.first->children.insert(pair.first->children.begin() + positions[i], originalNode);
                } else {
                    pair.first->addChild(originalNode);
                }
            }
        }
        
        // Restore connections
        if (map) {
            for (const auto& conn : removedConnections) {
                map->connections.push_back(conn);
            }
        }
        
        executed = false;
    }
}

std::string CutMultipleNodesCommand::getName() const {
    return _("Cut Multiple Nodes");
}

const std::vector<std::shared_ptr<Node>>& CutMultipleNodesCommand::getNodesCopy() const {
    return nodesCopy;
}

const std::vector<std::shared_ptr<Connection>>& CutMultipleNodesCommand::getConnectionsCopy() const {
    return connectionsCopy;
}

// PasteMultipleNodesCommand
PasteMultipleNodesCommand::PasteMultipleNodesCommand(std::shared_ptr<MindMap> m, std::shared_ptr<Node> parentNode, 
                                                     const std::vector<std::shared_ptr<Node>>& nodes,
                                                     const std::vector<std::shared_ptr<Connection>>& conns)
    : map(m), parent(parentNode), nodesToPaste(nodes), connectionsToPaste(conns), executed(false) {}

void PasteMultipleNodesCommand::execute() {
    if (!executed && parent && !nodesToPaste.empty()) {
        actualPastedNodes.clear();
        actualPastedConnections.clear();

        // Map clipboard nodes (input) to pasted nodes (output)
        std::map<std::shared_ptr<Node>, std::shared_ptr<Node>> clipboardToPasted;

        // Helper to populate map recursively
        std::function<void(std::shared_ptr<Node>, std::shared_ptr<Node>)> mapNodes = 
            [&](std::shared_ptr<Node> clip, std::shared_ptr<Node> pasted) {
                clipboardToPasted[clip] = pasted;
                if (clip->children.size() == pasted->children.size()) {
                    for (size_t i = 0; i < clip->children.size(); ++i) {
                        mapNodes(clip->children[i], pasted->children[i]);
                    }
                }
            };

        // Copy each node to paste and add to parent
        for (auto& node : nodesToPaste) {
            if (node) {
                auto nodeCopy = cloneNodeTree(node);
                if (nodeCopy) {
                    mapNodes(node, nodeCopy);
                    
                    // Apply intelligent positioning to avoid overlapping with existing children
                    if (nodeCopy->manualPosition) {
                        // Calculate a suitable position that doesn't overlap with existing children or other pasted nodes
                        std::pair<double, double> newPos = findNonOverlappingPosition(parent, nodeCopy, actualPastedNodes);
                        nodeCopy->x = newPos.first;
                        nodeCopy->y = newPos.second;
                    }

                    parent->addChild(nodeCopy);
                    actualPastedNodes.push_back(nodeCopy);
                }
            }
        }
        
        // Reconstruct connections
        if (map && !connectionsToPaste.empty()) {
            for (const auto& conn : connectionsToPaste) {
                auto itFrom = clipboardToPasted.find(conn->from);
                auto itTo = clipboardToPasted.find(conn->to);
                
                if (itFrom != clipboardToPasted.end() && itTo != clipboardToPasted.end()) {
                    auto newConn = std::make_shared<Connection>(*conn);
                    newConn->from = itFrom->second;
                    newConn->to = itTo->second;
                    // Generate new ID for the new connection
                    newConn->id = Connection::generateId();
                    
                    map->connections.push_back(newConn);
                    actualPastedConnections.push_back(newConn);
                }
            }
        }
        
        executed = true;
    }
}

void PasteMultipleNodesCommand::undo() {
    if (executed && parent) {
        // Remove all pasted nodes from parent
        for (auto& node : actualPastedNodes) {
            if (node) {
                parent->removeChild(node);
            }
        }
        
        // Remove pasted connections from map
        if (map) {
            std::vector<std::shared_ptr<Connection>> keptConnections;
            for (const auto& conn : map->connections) {
                bool isPasted = false;
                for (const auto& pastedConn : actualPastedConnections) {
                    if (conn == pastedConn) {
                        isPasted = true;
                        break;
                    }
                }
                if (!isPasted) {
                    keptConnections.push_back(conn);
                }
            }
            map->connections = keptConnections;
        }
        
        actualPastedNodes.clear();
        actualPastedConnections.clear();
        executed = false;
    }
}

std::string PasteMultipleNodesCommand::getName() const {
    return _("Paste Multiple Nodes");
}

const std::vector<std::shared_ptr<Node>>& PasteMultipleNodesCommand::getPastedNodes() const {
    return actualPastedNodes;
}

std::pair<double, double> PasteMultipleNodesCommand::findNonOverlappingPosition(std::shared_ptr<Node> targetParent, std::shared_ptr<Node> nodeToPaste, const std::vector<std::shared_ptr<Node>>& otherPastedNodes) {
    // Calculate the offset that should be applied to the entire subtree
    double rootOriginalX = nodeToPaste->x;
    double rootOriginalY = nodeToPaste->y;

    // Start with a basic offset from the original root position
    double newRootX = rootOriginalX + 40.0;
    double newRootY = rootOriginalY + 40.0;

    // Get a list of all children positions of the target parent to check for overlaps
    std::vector<std::pair<double, double>> existingPositions;
    for (const auto& child : targetParent->children) {
        existingPositions.push_back({child->x, child->y});
    }

    // Also check positions of other nodes being pasted in this operation
    for (const auto& pastedNode : otherPastedNodes) {
        existingPositions.push_back({pastedNode->x, pastedNode->y});
    }

    // Keep trying different positions until we find one that doesn't overlap
    int attempts = 0;
    const int maxAttempts = 100; // Prevent infinite loops

    while (attempts < maxAttempts) {
        bool overlaps = false;

        // Check if the new root position overlaps with any existing child or other pasted node
        for (const auto& pos : existingPositions) {
            // Simple distance check - if closer than a threshold, consider it overlapping
            double distance = std::sqrt(std::pow(newRootX - pos.first, 2) + std::pow(newRootY - pos.second, 2));
            if (distance < 60.0) { // 60 pixel minimum distance
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            break; // Found a good position
        }

        // Try a different position using a spiral pattern
        attempts++;
        double angle = attempts * 0.785; // Approximately 45 degrees in radians
        double radius = (attempts / 8) * 60.0; // Increase radius every 8 attempts
        newRootX = targetParent->x + radius * std::cos(angle);
        newRootY = targetParent->y + radius * std::sin(angle);
    }

    // If we've exhausted attempts, use a fallback position
    if (attempts >= maxAttempts) {
        newRootX = targetParent->x + 100.0 * (otherPastedNodes.size() + 1);
        newRootY = targetParent->y + 100.0;
    }

    // Now apply the calculated offset to the entire subtree
    double offsetX = newRootX - rootOriginalX;
    double offsetY = newRootY - rootOriginalY;

    // Apply the offset to the root node
    nodeToPaste->x = newRootX;
    nodeToPaste->y = newRootY;

    // Apply the same offset to all descendants in the subtree
    applyOffsetToSubtree(nodeToPaste, offsetX, offsetY);

    // Return the new root position
    return std::make_pair(newRootX, newRootY);
}

void PasteMultipleNodesCommand::applyOffsetToSubtree(std::shared_ptr<Node> node, double offsetX, double offsetY) {
    if (!node) return;

    for (auto& child : node->children) {
        child->x += offsetX;
        child->y += offsetY;

        // Recursively apply offset to grandchildren
        applyOffsetToSubtree(child, offsetX, offsetY);
    }
}
