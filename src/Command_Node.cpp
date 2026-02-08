#include "Command.hpp"
#include "Translation.hpp"
#include "MindMapUtils.hpp" // For cloneNodeTree if needed, though it is in MindMap.hpp
#include <algorithm>
#include <functional>

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
RemoveNodeCommand::RemoveNodeCommand(std::shared_ptr<MindMap> m, std::shared_ptr<Node> parentNode, std::shared_ptr<Node> nodeToRemove) 
    : map(m), parent(parentNode), node(nodeToRemove), nodeRef(nodeToRemove), executed(true) {
    // Find the position of the node in parent's children
    if (parent) {
        auto it = std::find(parent->children.begin(), parent->children.end(), node);
        if (it != parent->children.end()) {
            position = std::distance(parent->children.begin(), it);
        }
    }

    // Backup connections related to this node or its descendants
    if (map && node) {
        std::vector<std::shared_ptr<Node>> subtreeNodes;
        std::function<void(std::shared_ptr<Node>)> collect = [&](std::shared_ptr<Node> n) {
            subtreeNodes.push_back(n);
            for(auto& c : n->children) collect(c);
        };
        collect(node);

        for (const auto& conn : map->connections) {
            bool related = false;
            for (const auto& n : subtreeNodes) {
                if (conn->from == n || conn->to == n) {
                    related = true;
                    break;
                }
            }
            if (related) {
                removedConnections.push_back(*conn);
            }
        }
    }
}

void RemoveNodeCommand::execute() {
    if (executed && parent && node) {
        parent->removeChild(node);
        
        // Remove related connections from the map
        if (map && !removedConnections.empty()) {
             std::vector<std::shared_ptr<Connection>> keptConnections;
             for (const auto& mapConn : map->connections) {
                 bool shouldRemove = false;
                 for (const auto& remConn : removedConnections) {
                     if (mapConn->id == remConn.id) {
                         shouldRemove = true;
                         break;
                     }
                 }
                 if (!shouldRemove) {
                     keptConnections.push_back(mapConn);
                 }
             }
             map->connections = keptConnections;
        }

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
        
        // Restore connections
        if (map) {
            for (const auto& connData : removedConnections) {
                auto newConn = std::make_shared<Connection>(connData);
                newConn->id = connData.id; 
                map->connections.push_back(newConn);
            }
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
                E4Color oldCol, E4Color newCol,
                E4Color oldTxtCol, E4Color newTxtCol,
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

// MoveMultipleNodesCommand
MoveMultipleNodesCommand::MoveMultipleNodesCommand(const std::vector<std::shared_ptr<Node>>& nodes, const std::vector<std::pair<double, double>>& oldPositions, const std::vector<std::pair<double, double>>& newPositions)
    : executed(false) {
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i < oldPositions.size() && i < newPositions.size()) {
            moves.push_back({nodes[i], oldPositions[i].first, oldPositions[i].second, newPositions[i].first, newPositions[i].second});
        }
    }
}

void MoveMultipleNodesCommand::execute() {
    if (!executed) {
        for (auto& move : moves) {
            if (move.node) {
                move.node->x = move.newX;
                move.node->y = move.newY;
                move.node->manualPosition = true;
            }
        }
        executed = true;
    }
}

void MoveMultipleNodesCommand::undo() {
    if (executed) {
        for (auto& move : moves) {
            if (move.node) {
                move.node->x = move.oldX;
                move.node->y = move.oldY;
            }
        }
        executed = false;
    }
}

std::string MoveMultipleNodesCommand::getName() const {
    return _("Move Multiple Nodes");
}
