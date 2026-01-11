#include "Command.hpp"
#include "Translation.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

// Helper function to deep copy a node and its children
// Internal linkage
namespace {
    std::shared_ptr<Node> copyNodeTree(std::shared_ptr<Node> original) {
        if (!original) return nullptr;

        auto copy = std::make_shared<Node>(original->text, original->color);
        copy->fontDesc = original->fontDesc;
        copy->imagePath = original->imagePath;
        copy->imgWidth = original->imgWidth;
        copy->imgHeight = original->imgHeight;
        copy->connText = original->connText;
        copy->connImagePath = original->connImagePath;
        copy->textColor = original->textColor;
        copy->x = original->x;
        copy->y = original->y;
        copy->width = original->width;
        copy->height = original->height;
        copy->angle = original->angle;
        copy->manualPosition = original->manualPosition;
        
        // Copy override flags
        copy->overrideColor = original->overrideColor;
        copy->overrideTextColor = original->overrideTextColor;
        copy->overrideFont = original->overrideFont;

        // Copy all children recursively
        for (auto& child : original->children) {
            auto childCopy = copyNodeTree(child);
            if (childCopy) {
                copy->addChild(childCopy);
            }
        }

        return copy;
    }
}

// AddNodeCommand
AddNodeCommand::AddNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> newNode) 
    : parent(parentNode), node(newNode), executed(false) {}

void AddNodeCommand::execute() {
    if (!executed && parent && node) {
        parent->addChild(node);
        executed = true;
    }
}

void AddNodeCommand::undo() {
    if (executed && parent && node) {
        parent->removeChild(node);
        executed = false;
    }
}

std::string AddNodeCommand::getName() const {
    return _("Add Node");
}

// RemoveNodeCommand
RemoveNodeCommand::RemoveNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToRemove) 
    : parent(parentNode), node(nodeToRemove), nodeRef(nodeToRemove), executed(true) {
    // Find the position of the node in parent's children
    if (parent) {
        auto it = std::find(parent->children.begin(), parent->children.end(), node);
        if (it != parent->children.end()) {
            position = std::distance(parent->children.begin(), it);
        }
    }
}

void RemoveNodeCommand::execute() {
    if (executed && parent && node) {
        parent->removeChild(node);
        executed = false;
    }
}

void RemoveNodeCommand::undo() {
    if (!executed && parent && !nodeRef.expired()) {
        // Reinsert the node at its original position
        if (position < parent->children.size()) {
            parent->children.insert(parent->children.begin() + position, nodeRef.lock());
        } else {
            parent->addChild(nodeRef.lock());
        }
        executed = true;
    }
}

std::string RemoveNodeCommand::getName() const {
    return _("Remove Node");
}

// EditNodeCommand
EditNodeCommand::EditNodeCommand(std::shared_ptr<Node> nodeToEdit,
                const std::string& oldTxt, const std::string& newTxt,
                const std::string& oldFont, const std::string& newFont,
                Color oldCol, Color newCol,
                Color oldTxtCol, Color newTxtCol,
                const std::string& oldImgPath, const std::string& newImgPath,
                int oldW, int newW, int oldH, int newH,
                const std::string& oldConnTxt, const std::string& newConnTxt,
                const std::string& oldConnImgPath, const std::string& newConnImgPath,
                const std::string& oldConnFont, const std::string& newConnFont,
                bool oldOc, bool newOc,
                bool oldOt, bool newOt,
                bool oldOf, bool newOf,
                bool oldOvrCf, bool newOvrCf)
    : node(nodeToEdit), oldText(oldTxt), newText(newTxt),
        oldFontDesc(oldFont), newFontDesc(newFont),
        oldColor(oldCol), newColor(newCol),
        oldTextColor(oldTxtCol), newTextColor(newTxtCol),
        oldImagePath(oldImgPath), newImagePath(newImgPath),
        oldImgWidth(oldW), newImgWidth(newW),
        oldImgHeight(oldH), newImgHeight(newH),
        oldConnText(oldConnTxt), newConnText(newConnTxt),
        oldConnImagePath(oldConnImgPath), newConnImagePath(newConnImgPath),
        oldConnFontDesc(oldConnFont), newConnFontDesc(newConnFont),
        oldOvrC(oldOc), newOvrC(newOc),
        oldOvrT(oldOt), newOvrT(newOt),
        oldOvrF(oldOf), newOvrF(newOf),
        oldOvrCF(oldOvrCf), newOvrCF(newOvrCf),
        executed(false) {}

void EditNodeCommand::execute() {
    if (!executed && node) {
        // Apply the new values
        node->text = newText;
        node->fontDesc = newFontDesc;
        node->color = newColor;
        node->textColor = newTextColor;
        node->imagePath = newImagePath;
        node->imgWidth = newImgWidth;
        node->imgHeight = newImgHeight;
        node->connText = newConnText;
        node->connImagePath = newConnImagePath;
        node->connFontDesc = newConnFontDesc;
        
        node->overrideColor = newOvrC;
        node->overrideTextColor = newOvrT;
        node->overrideFont = newOvrF;
        node->overrideConnFont = newOvrCF;
        
        executed = true;
    }
}

void EditNodeCommand::undo() {
    if (executed && node) {
        // Revert to old values
        node->text = oldText;
        node->fontDesc = oldFontDesc;
        node->color = oldColor;
        node->textColor = oldTextColor;
        node->imagePath = oldImagePath;
        node->imgWidth = oldImgWidth;
        node->imgHeight = oldImgHeight;
        node->connText = oldConnText;
        node->connImagePath = oldConnImagePath;
        node->connFontDesc = oldConnFontDesc;
        
        node->overrideColor = oldOvrC;
        node->overrideTextColor = oldOvrT;
        node->overrideFont = oldOvrF;
        node->overrideConnFont = oldOvrCF;
        
        executed = false;
    }
}

std::string EditNodeCommand::getName() const {
    return _("Edit Node");
}

// MoveNodeCommand
MoveNodeCommand::MoveNodeCommand(std::shared_ptr<Node> nodeToMove, double oldXPos, double oldYPos, double newXPos, double newYPos)
    : node(nodeToMove), oldX(oldXPos), oldY(oldYPos), newX(newXPos), newY(newYPos), executed(false) {}

void MoveNodeCommand::execute() {
    if (!executed && node) {
        node->x = newX;
        node->y = newY;
        node->manualPosition = true;
        executed = true;
    }
}

void MoveNodeCommand::undo() {
    if (executed && node) {
        node->x = oldX;
        node->y = oldY;
        executed = false;
    }
}

std::string MoveNodeCommand::getName() const {
    return _("Move Node");
}

// CopyNodeCommand
CopyNodeCommand::CopyNodeCommand(std::shared_ptr<Node> node)
    : nodeToCopy(node), executed(false) {}

void CopyNodeCommand::execute() {
    if (!executed && nodeToCopy) {
        nodeCopy = copyNodeTree(nodeToCopy);
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
            nodeCopy = copyNodeTree(nodeToCut);  // Keep a copy before removal
            parent->removeChild(nodeToCut);
            executed = true;
        }
    }
}

void CutNodeCommand::undo() {
    if (executed && parent && nodeCopy) {
        // Reinsert the node copy at its original position
        if (position < parent->children.size()) {
            parent->children.insert(parent->children.begin() + position, copyNodeTree(nodeCopy));
        } else {
            parent->addChild(copyNodeTree(nodeCopy));
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
        actualPastedNode = copyNodeTree(nodeToPaste);
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
CopyMultipleNodesCommand::CopyMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes)
    : nodesToCopy(nodes), executed(false) {}

void CopyMultipleNodesCommand::execute() {
    if (!executed) {
        for (auto& node : nodesToCopy) {
            if (node) {
                nodesCopy.push_back(copyNodeTree(node));
            }
        }
        executed = true;
    }
}

void CopyMultipleNodesCommand::undo() {
    // Copy operation doesn't modify the map, so undo is a no-op
    if (executed) {
        nodesCopy.clear();
        executed = false;
    }
}

std::string CopyMultipleNodesCommand::getName() const {
    return _("Copy Multiple Nodes");
}

const std::vector<std::shared_ptr<Node>>& CopyMultipleNodesCommand::getNodesCopy() const {
    return nodesCopy;
}

// CutMultipleNodesCommand
CutMultipleNodesCommand::CutMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes) : executed(false) {
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
        // Keep copies of nodes before removal
        for (auto& node : parentChildPairs) {
            nodesCopy.push_back(copyNodeTree(node.second));
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
                auto nodeToRestore = copyNodeTree(nodesCopy[i]);
                if (!nodeToRestore) continue;

                // Reinsert at original position
                if (positions[i] < pair.first->children.size()) {
                    pair.first->children.insert(pair.first->children.begin() + positions[i], nodeToRestore);
                } else {
                    pair.first->addChild(nodeToRestore);
                }
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

// PasteMultipleNodesCommand
PasteMultipleNodesCommand::PasteMultipleNodesCommand(std::shared_ptr<Node> parentNode, const std::vector<std::shared_ptr<Node>>& nodes)
    : parent(parentNode), nodesToPaste(nodes), executed(false) {}

void PasteMultipleNodesCommand::execute() {
    if (!executed && parent && !nodesToPaste.empty()) {
        actualPastedNodes.clear();

        // Copy each node to paste and add to parent
        for (auto& node : nodesToPaste) {
            if (node) {
                auto nodeCopy = copyNodeTree(node);
                if (nodeCopy) {
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
        executed = true;
    }
}

void PasteMultipleNodesCommand::undo() {
    if (executed && parent && !actualPastedNodes.empty()) {
        // Remove all pasted nodes from parent
        for (auto& node : actualPastedNodes) {
            if (node) {
                parent->removeChild(node);
            }
        }
        actualPastedNodes.clear();
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

// RemoveConnectionCommand
RemoveConnectionCommand::RemoveConnectionCommand(std::shared_ptr<MindMap> m, std::shared_ptr<Node> f, std::shared_ptr<Node> t)
    : map(m), from(f), to(t), connectionCopy(f, t), executed(false) {
    // Try to find existing connection to copy its properties for undo
    if (map) {
        for (const auto& conn : map->connections) {
            if (conn.from == from && conn.to == to) {
                connectionCopy = conn; // Store copy
                break;
            }
        }
    }
}

void RemoveConnectionCommand::execute() {
    if (!executed && map) {
        map->removeConnection(from, to);
        executed = true;
    }
}

void RemoveConnectionCommand::undo() {
    if (executed && map) {
        // Restore connection
        map->connections.push_back(connectionCopy);
        executed = false;
    }
}

std::string RemoveConnectionCommand::getName() const {
    return _("Remove Connection");
}

// CommandManager
void CommandManager::executeCommand(std::unique_ptr<Command> cmd) {
    cmd->execute();
    
    // Add to undo stack
    undoStack.push(std::move(cmd));
    
    // Clear redo stack since we're branching from history
    while (!redoStack.empty()) {
        redoStack.pop();
    }
    
    // Limit the size of the undo stack
    if (undoStack.size() > MAX_COMMANDS) {
        std::stack<std::unique_ptr<Command>> tempStack;
        // Move the most recent MAX_COMMANDS/2 commands to the temp stack
        int count = 0;
        while (!undoStack.empty() && count < MAX_COMMANDS/2) {
            tempStack.push(std::move(undoStack.top()));
            undoStack.pop();
            count++;
        }
        // Move them back to undo stack in correct order
        while (!tempStack.empty()) {
            undoStack.push(std::move(tempStack.top()));
            tempStack.pop();
        }
    }
}

bool CommandManager::canUndo() const {
    return !undoStack.empty();
}

bool CommandManager::canRedo() const {
    return !redoStack.empty();
}

void CommandManager::undo() {
    if (!undoStack.empty()) {
        auto cmd = std::move(undoStack.top());
        cmd->undo();
        undoStack.pop();
        redoStack.push(std::move(cmd));
    }
}

void CommandManager::redo() {
    if (!redoStack.empty()) {
        auto cmd = std::move(redoStack.top());
        cmd->execute();
        redoStack.pop();
        undoStack.push(std::move(cmd));
    }
}

std::string CommandManager::getUndoName() const {
    if (!undoStack.empty()) {
        return undoStack.top()->getName();
    }
    return "";
}

std::string CommandManager::getRedoName() const {
    if (!redoStack.empty()) {
        return redoStack.top()->getName();
    }
    return "";
}

void CommandManager::clear() {
    while (!undoStack.empty()) {
        undoStack.pop();
    }
    while (!redoStack.empty()) {
        redoStack.pop();
    }
}
