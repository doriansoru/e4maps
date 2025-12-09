#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "MindMap.hpp"
#include <memory>
#include <string>
#include "Translation.hpp"
#include "Constants.hpp"

// Base Command interface
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getName() const = 0;
};

// Command to add a node
class AddNodeCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::shared_ptr<Node> node;
    bool executed;

public:
    AddNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> newNode) 
        : parent(parentNode), node(newNode), executed(false) {}

    void execute() override {
        if (!executed && parent && node) {
            parent->addChild(node);
            executed = true;
        }
    }

    void undo() override {
        if (executed && parent && node) {
            parent->removeChild(node);
            executed = false;
        }
    }

    std::string getName() const override {
        return _("Add Node");
    }
};

// Command to remove a node
class RemoveNodeCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::shared_ptr<Node> node;
    std::weak_ptr<Node> nodeRef;
    std::size_t position;
    bool executed;

public:
    RemoveNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToRemove) 
        : parent(parentNode), node(nodeToRemove), nodeRef(nodeToRemove), executed(true) {
        // Find the position of the node in parent's children
        if (parent) {
            auto it = std::find(parent->children.begin(), parent->children.end(), node);
            if (it != parent->children.end()) {
                position = std::distance(parent->children.begin(), it);
            }
        }
    }

    void execute() override {
        if (executed && parent && node) {
            parent->removeChild(node);
            executed = false;
        }
    }

    void undo() override {
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

    std::string getName() const override {
        return _("Remove Node");
    }
};

// Command to edit a node
class EditNodeCommand : public Command {
private:
    std::shared_ptr<Node> node;
    std::string oldText, newText;
    std::string oldFontDesc, newFontDesc;
    Color oldColor, newColor;
    Color oldTextColor, newTextColor;
    std::string oldImagePath, newImagePath;
    int oldImgWidth, newImgWidth;
    int oldImgHeight, newImgHeight;
    std::string oldConnText, newConnText;
    std::string oldConnImagePath, newConnImagePath;
    
    // Override flags
    bool oldOvrC, newOvrC;
    bool oldOvrT, newOvrT;
    bool oldOvrF, newOvrF;
    
    bool executed;

public:
    EditNodeCommand(std::shared_ptr<Node> nodeToEdit,
                    const std::string& oldTxt, const std::string& newTxt,
                    const std::string& oldFont, const std::string& newFont,
                    Color oldCol, Color newCol,
                    Color oldTxtCol, Color newTxtCol,
                    const std::string& oldImgPath, const std::string& newImgPath,
                    int oldW, int newW, int oldH, int newH,
                    const std::string& oldConnTxt, const std::string& newConnTxt,
                    const std::string& oldConnImgPath, const std::string& newConnImgPath,
                    bool oldOc, bool newOc,
                    bool oldOt, bool newOt,
                    bool oldOf, bool newOf)
        : node(nodeToEdit), oldText(oldTxt), newText(newTxt),
          oldFontDesc(oldFont), newFontDesc(newFont),
          oldColor(oldCol), newColor(newCol),
          oldTextColor(oldTxtCol), newTextColor(newTxtCol),
          oldImagePath(oldImgPath), newImagePath(newImgPath),
          oldImgWidth(oldW), newImgWidth(newW),
          oldImgHeight(oldH), newImgHeight(newH),
          oldConnText(oldConnTxt), newConnText(newConnTxt),
          oldConnImagePath(oldConnImgPath), newConnImagePath(newConnImgPath),
          oldOvrC(oldOc), newOvrC(newOc),
          oldOvrT(oldOt), newOvrT(newOt),
          oldOvrF(oldOf), newOvrF(newOf),
          executed(false) {}

    void execute() override {
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
            
            node->overrideColor = newOvrC;
            node->overrideTextColor = newOvrT;
            node->overrideFont = newOvrF;
            
            executed = true;
        }
    }

    void undo() override {
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
            
            node->overrideColor = oldOvrC;
            node->overrideTextColor = oldOvrT;
            node->overrideFont = oldOvrF;
            
            executed = false;
        }
    }

    std::string getName() const override {
        return _("Edit Node");
    }
};

// Helper function to deep copy a node and its children
inline std::shared_ptr<Node> copyNodeTree(std::shared_ptr<Node> original) {
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

// Command to move a node
class MoveNodeCommand : public Command {
private:
    std::shared_ptr<Node> node;
    double oldX, oldY;
    double newX, newY;
    bool executed;

public:
    MoveNodeCommand(std::shared_ptr<Node> nodeToMove, double oldXPos, double oldYPos, double newXPos, double newYPos)
        : node(nodeToMove), oldX(oldXPos), oldY(oldYPos), newX(newXPos), newY(newYPos), executed(false) {}

    void execute() override {
        if (!executed && node) {
            node->x = newX;
            node->y = newY;
            node->manualPosition = true;
            executed = true;
        }
    }

    void undo() override {
        if (executed && node) {
            node->x = oldX;
            node->y = oldY;
            executed = false;
        }
    }

    std::string getName() const override {
        return _("Move Node");
    }
};

// Command to copy a node (doesn't modify the map, just stores a copy)
class CopyNodeCommand : public Command {
private:
    std::shared_ptr<Node> nodeToCopy;
    std::shared_ptr<Node> nodeCopy;
    bool executed;

public:
    CopyNodeCommand(std::shared_ptr<Node> node)
        : nodeToCopy(node), executed(false) {}

    void execute() override {
        if (!executed && nodeToCopy) {
            nodeCopy = copyNodeTree(nodeToCopy);
            executed = true;
        }
    }

    void undo() override {
        // Copy operation doesn't modify the map, so undo is a no-op
        if (executed) {
            nodeCopy.reset();
            executed = false;
        }
    }

    std::string getName() const override {
        return _("Copy Node");
    }

    std::shared_ptr<Node> getNodeCopy() const {
        return nodeCopy;
    }
};

// Command to cut a node (remove from map but keep a copy)
class CutNodeCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::shared_ptr<Node> nodeToCut;
    std::shared_ptr<Node> nodeCopy;
    std::size_t position;
    bool executed;

public:
    CutNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToCut)
        : parent(parentNode), nodeToCut(nodeToCut), executed(false) {}

    void execute() override {
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

    void undo() override {
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

    std::string getName() const override {
        return _("Cut Node");
    }

    std::shared_ptr<Node> getNodeCopy() const {
        return nodeCopy;
    }
};

// Command to paste a node as child of another node
class PasteNodeCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::shared_ptr<Node> nodeToPaste;
    std::shared_ptr<Node> actualPastedNode;  // The instance that gets added to the map
    bool executed;

public:
    PasteNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToPaste)
        : parent(parentNode), nodeToPaste(nodeToPaste), executed(false) {}

    void execute() override {
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

    void undo() override {
        if (executed && parent && actualPastedNode) {
            parent->removeChild(actualPastedNode);
            executed = false;
        }
    }

    std::string getName() const override {
        return _("Paste Node");
    }

private:
    // Helper method to find a non-overlapping position for the pasted node and its subtree
    std::pair<double, double> findNonOverlappingPosition(std::shared_ptr<Node> targetParent, std::shared_ptr<Node> nodeToPaste) {
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

    // Helper method to apply an offset to all nodes in a subtree
    void applyOffsetToSubtree(std::shared_ptr<Node> node, double offsetX, double offsetY) {
        if (!node) return;

        for (auto& child : node->children) {
            child->x += offsetX;
            child->y += offsetY;

            // Recursively apply offset to grandchildren
            applyOffsetToSubtree(child, offsetX, offsetY);
        }
    }
};

// Command to copy multiple nodes (doesn't modify the map, just stores multiple copies)
class CopyMultipleNodesCommand : public Command {
private:
    std::vector<std::shared_ptr<Node>> nodesToCopy;
    std::vector<std::shared_ptr<Node>> nodesCopy;
    bool executed;

public:
    CopyMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes)
        : nodesToCopy(nodes), executed(false) {}

    void execute() override {
        if (!executed) {
            for (auto& node : nodesToCopy) {
                if (node) {
                    nodesCopy.push_back(copyNodeTree(node));
                }
            }
            executed = true;
        }
    }

    void undo() override {
        // Copy operation doesn't modify the map, so undo is a no-op
        if (executed) {
            nodesCopy.clear();
            executed = false;
        }
    }

    std::string getName() const override {
        return _("Copy Multiple Nodes");
    }

    const std::vector<std::shared_ptr<Node>>& getNodesCopy() const {
        return nodesCopy;
    }
};

// Command to cut multiple nodes (remove from map but keep copies)
class CutMultipleNodesCommand : public Command {
private:
    std::vector<std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>>> parentChildPairs; // parent, child
    std::vector<std::shared_ptr<Node>> nodesCopy;
    std::vector<std::size_t> positions; // positions of nodes in their respective parents' children
    bool executed;

public:
    CutMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes) : executed(false) {
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

    void execute() override {
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

    void undo() override {
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

    std::string getName() const override {
        return _("Cut Multiple Nodes");
    }

    const std::vector<std::shared_ptr<Node>>& getNodesCopy() const {
        return nodesCopy;
    }
};

// Command to paste multiple nodes to a parent
class PasteMultipleNodesCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> nodesToPaste;
    std::vector<std::shared_ptr<Node>> actualPastedNodes; // Copied instances that get added to the map
    bool executed;

public:
    PasteMultipleNodesCommand(std::shared_ptr<Node> parentNode, const std::vector<std::shared_ptr<Node>>& nodes)
        : parent(parentNode), nodesToPaste(nodes), executed(false) {}

    void execute() override {
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

    void undo() override {
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

    std::string getName() const override {
        return _("Paste Multiple Nodes");
    }

    const std::vector<std::shared_ptr<Node>>& getPastedNodes() const {
        return actualPastedNodes;
    }

private:
    // Helper method to find a non-overlapping position for the pasted node and its subtree
    std::pair<double, double> findNonOverlappingPosition(std::shared_ptr<Node> targetParent, std::shared_ptr<Node> nodeToPaste, const std::vector<std::shared_ptr<Node>>& otherPastedNodes) {
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

    // Helper method to apply an offset to all nodes in a subtree
    void applyOffsetToSubtree(std::shared_ptr<Node> node, double offsetX, double offsetY) {
        if (!node) return;

        for (auto& child : node->children) {
            child->x += offsetX;
            child->y += offsetY;

            // Recursively apply offset to grandchildren
            applyOffsetToSubtree(child, offsetX, offsetY);
        }
    }
};

// Command Manager to handle undo/redo
class CommandManager {
private:
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    static const size_t MAX_COMMANDS = E4Maps::MAX_COMMAND_HISTORY; // Limit command history

public:
    void executeCommand(std::unique_ptr<Command> cmd) {
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

    bool canUndo() const {
        return !undoStack.empty();
    }

    bool canRedo() const {
        return !redoStack.empty();
    }

    void undo() {
        if (!undoStack.empty()) {
            auto cmd = std::move(undoStack.top());
            cmd->undo();
            undoStack.pop();
            redoStack.push(std::move(cmd));
        }
    }

    void redo() {
        if (!redoStack.empty()) {
            auto cmd = std::move(redoStack.top());
            cmd->execute();
            redoStack.pop();
            undoStack.push(std::move(cmd));
        }
    }

    std::string getUndoName() const {
        if (!undoStack.empty()) {
            return undoStack.top()->getName();
        }
        return "";
    }

    std::string getRedoName() const {
        if (!redoStack.empty()) {
            return redoStack.top()->getName();
        }
        return "";
    }

    void clear() {
        while (!undoStack.empty()) {
            undoStack.pop();
        }
        while (!redoStack.empty()) {
            redoStack.pop();
        }
    }
};

#endif // COMMAND_HPP