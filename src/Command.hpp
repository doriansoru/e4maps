#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "MindMap.hpp"
#include <memory>
#include <string>
#include <stack>
#include <vector>
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
    AddNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> newNode);
    void execute() override;
    void undo() override;
    std::string getName() const override;
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
    RemoveNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToRemove);
    void execute() override;
    void undo() override;
    std::string getName() const override;
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
    std::string oldConnFontDesc, newConnFontDesc;
    
    // Override flags
    bool oldOvrC, newOvrC;
    bool oldOvrT, newOvrT;
    bool oldOvrF, newOvrF;
    bool oldOvrCF, newOvrCF;
    
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
                    const std::string& oldConnFont, const std::string& newConnFont,
                    bool oldOc, bool newOc,
                    bool oldOt, bool newOt,
                    bool oldOf, bool newOf,
                    bool oldOvrCf, bool newOvrCf);

    void execute() override;
    void undo() override;
    std::string getName() const override;
};

// Command to move a node
class MoveNodeCommand : public Command {
private:
    std::shared_ptr<Node> node;
    double oldX, oldY;
    double newX, newY;
    bool executed;

public:
    MoveNodeCommand(std::shared_ptr<Node> nodeToMove, double oldXPos, double oldYPos, double newXPos, double newYPos);
    void execute() override;
    void undo() override;
    std::string getName() const override;
};

// Command to copy a node (doesn't modify the map, just stores a copy)
class CopyNodeCommand : public Command {
private:
    std::shared_ptr<Node> nodeToCopy;
    std::shared_ptr<Node> nodeCopy;
    bool executed;

public:
    CopyNodeCommand(std::shared_ptr<Node> node);
    void execute() override;
    void undo() override;
    std::string getName() const override;
    std::shared_ptr<Node> getNodeCopy() const;
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
    CutNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToCut);
    void execute() override;
    void undo() override;
    std::string getName() const override;
    std::shared_ptr<Node> getNodeCopy() const;
};

// Command to paste a node as child of another node
class PasteNodeCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::shared_ptr<Node> nodeToPaste;
    std::shared_ptr<Node> actualPastedNode;  // The instance that gets added to the map
    bool executed;

    std::pair<double, double> findNonOverlappingPosition(std::shared_ptr<Node> targetParent, std::shared_ptr<Node> nodeToPaste);
    void applyOffsetToSubtree(std::shared_ptr<Node> node, double offsetX, double offsetY);

public:
    PasteNodeCommand(std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToPaste);
    void execute() override;
    void undo() override;
    std::string getName() const override;
};

// Command to copy multiple nodes
class CopyMultipleNodesCommand : public Command {
private:
    std::vector<std::shared_ptr<Node>> nodesToCopy;
    std::vector<std::shared_ptr<Node>> nodesCopy;
    bool executed;

public:
    CopyMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes);
    void execute() override;
    void undo() override;
    std::string getName() const override;
    const std::vector<std::shared_ptr<Node>>& getNodesCopy() const;
};

// Command to cut multiple nodes
class CutMultipleNodesCommand : public Command {
private:
    std::vector<std::pair<std::shared_ptr<Node>, std::shared_ptr<Node>>> parentChildPairs; // parent, child
    std::vector<std::shared_ptr<Node>> nodesCopy;
    std::vector<std::size_t> positions; // positions of nodes in their respective parents' children
    bool executed;

public:
    CutMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes);
    void execute() override;
    void undo() override;
    std::string getName() const override;
    const std::vector<std::shared_ptr<Node>>& getNodesCopy() const;
};

// Command to paste multiple nodes to a parent
class PasteMultipleNodesCommand : public Command {
private:
    std::shared_ptr<Node> parent;
    std::vector<std::shared_ptr<Node>> nodesToPaste;
    std::vector<std::shared_ptr<Node>> actualPastedNodes; // Copied instances that get added to the map
    bool executed;

    std::pair<double, double> findNonOverlappingPosition(std::shared_ptr<Node> targetParent, std::shared_ptr<Node> nodeToPaste, const std::vector<std::shared_ptr<Node>>& otherPastedNodes);
    void applyOffsetToSubtree(std::shared_ptr<Node> node, double offsetX, double offsetY);

public:
    PasteMultipleNodesCommand(std::shared_ptr<Node> parentNode, const std::vector<std::shared_ptr<Node>>& nodes);
    void execute() override;
    void undo() override;
    std::string getName() const override;
    const std::vector<std::shared_ptr<Node>>& getPastedNodes() const;
};

// Command Manager to handle undo/redo
class CommandManager {
private:
    std::stack<std::unique_ptr<Command>> undoStack;
    std::stack<std::unique_ptr<Command>> redoStack;
    static const size_t MAX_COMMANDS = E4Maps::MAX_COMMAND_HISTORY; // Limit command history

public:
    void executeCommand(std::unique_ptr<Command> cmd);
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    std::string getUndoName() const;
    std::string getRedoName() const;
    void clear();
};

#endif // COMMAND_HPP
