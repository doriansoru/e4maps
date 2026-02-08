#include "MapController.hpp"
#include "MapSerializer.hpp"
#include "Translation.hpp"
#include "MindMapUtils.hpp"
#include <iostream>

MapController::MapController() {
    newMap(); // Initialize with a new map
}

void MapController::newMap() {
    m_Map = std::make_shared<MindMap>(_("MAIN IDEA"));
    m_currentFilename.clear();
    m_commandManager.clear();
    setModified(false);
    signal_map_changed.emit();
}

bool MapController::loadMap(const std::string& path) {
    try {
        auto newMap = MapSerializer::load(path);
        if (!newMap || !newMap->root) {
            return false;
        }
        m_Map = newMap;
        m_currentFilename = path;
        m_commandManager.clear();
        setModified(false);
        signal_map_changed.emit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading map: " << e.what() << std::endl;
        return false;
    }
}

bool MapController::saveMap(const std::string& path) {
    try {
        MapSerializer::save(m_Map, path);
        m_currentFilename = path;
        setModified(false);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving map: " << e.what() << std::endl;
        return false;
    }
}

void MapController::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        signal_modified_changed.emit(m_modified);
    }
}

void MapController::executeCommand(std::unique_ptr<Command> cmd) {
    m_commandManager.executeCommand(std::move(cmd));
    setModified(true);
    // Note: The command execution usually updates the model,
    // so the view needs to know.
    // However, some commands (like layout) might not strictly require layout invalidation if they handle it,
    // but generally any model change implies layout invalidation.
    // The View usually listens to this or we emit a signal here.
    // We don't emit layout_invalidated here explicitly because often the View calls this method
    // and knows it needs to update. But for Undo/Redo it's crucial.
}

void MapController::undo() {
    if (canUndo()) {
        m_commandManager.undo();
        setModified(true);
        signal_layout_invalidated.emit();
    }
}

void MapController::redo() {
    if (canRedo()) {
        m_commandManager.redo();
        setModified(true);
        signal_layout_invalidated.emit();
    }
}

bool MapController::canUndo() const {
    return m_commandManager.canUndo();
}

bool MapController::canRedo() const {
    return m_commandManager.canRedo();
}

void MapController::clearHistory() {
    m_commandManager.clear();
}

void MapController::copy(const std::vector<std::shared_ptr<Node>>& nodes) {
    if (nodes.empty()) return;
    
    // Create a copy command to populate the clipboard
    auto copyCmd = std::make_unique<CopyMultipleNodesCommand>(m_Map, nodes);
    copyCmd->execute();

    m_clipboard = copyCmd->getNodesCopy();
    m_clipboardConnections = copyCmd->getConnectionsCopy();
}

void MapController::cut(const std::vector<std::shared_ptr<Node>>& nodes) {
    if (nodes.empty()) return;

    // Filter out root nodes
    std::vector<std::shared_ptr<Node>> nodesToCut;
    for (const auto& node : nodes) {
        if (node && !node->isRoot()) {
            nodesToCut.push_back(node);
        }
    }

    if (nodesToCut.empty()) return;

    auto cutCmd = std::make_unique<CutMultipleNodesCommand>(m_Map, nodesToCut);
    // Keep pointer to extract data after execution (before move)
    auto* cutCmdPtr = cutCmd.get();
    
    executeCommand(std::move(cutCmd));

    m_clipboard = cutCmdPtr->getNodesCopy();
    m_clipboardConnections = cutCmdPtr->getConnectionsCopy();
    
    signal_layout_invalidated.emit();
}

void MapController::paste(std::shared_ptr<Node> target) {
    if (!target || m_clipboard.empty()) return;

    auto pasteCmd = std::make_unique<PasteMultipleNodesCommand>(m_Map, target, m_clipboard, m_clipboardConnections);
    auto* pasteCmdPtr = pasteCmd.get();
    
    executeCommand(std::move(pasteCmd));
    
    signal_layout_invalidated.emit();
    
    // Ideally we would return the pasted nodes so the view can select them,
    // but the controller signature is void.
    // The view can access the last command if needed, or we could change the signature.
}

bool MapController::hasClipboardContent() const {
    return !m_clipboard.empty();
}
