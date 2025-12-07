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
                    const std::string& oldConnImgPath, const std::string& newConnImgPath)
        : node(nodeToEdit), oldText(oldTxt), newText(newTxt),
          oldFontDesc(oldFont), newFontDesc(newFont),
          oldColor(oldCol), newColor(newCol),
          oldTextColor(oldTxtCol), newTextColor(newTxtCol),
          oldImagePath(oldImgPath), newImagePath(newImgPath),
          oldImgWidth(oldW), newImgWidth(newW),
          oldImgHeight(oldH), newImgHeight(newH),
          oldConnText(oldConnTxt), newConnText(newConnTxt),
          oldConnImagePath(oldConnImgPath), newConnImagePath(newConnImgPath),
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