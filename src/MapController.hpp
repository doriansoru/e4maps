#ifndef MAPCONTROLLER_HPP
#define MAPCONTROLLER_HPP

#include <memory>
#include <string>
#include <vector>
#include <sigc++/sigc++.h>
#include "MindMap.hpp"
#include "Command.hpp"

class MapController {
public:
    MapController();
    
    // File Operations
    void newMap();
    bool loadMap(const std::string& path); // Returns true on success
    bool saveMap(const std::string& path); // Returns true on success
    
    // State Accessors
    std::shared_ptr<MindMap> getMap() const { return m_Map; }
    std::string getFilename() const { return m_currentFilename; }
    bool isModified() const { return m_modified; }
    void setModified(bool modified);
    
    // Command Management
    void executeCommand(std::unique_ptr<Command> cmd);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    void clearHistory(); // Clear undo/redo stack

    // Clipboard Operations
    void copy(const std::vector<std::shared_ptr<Node>>& nodes);
    void cut(const std::vector<std::shared_ptr<Node>>& nodes);
    void paste(std::shared_ptr<Node> target);
    bool hasClipboardContent() const;
    
    // Signals
    // Emitted when a new map is loaded/created (replaces the old one)
    sigc::signal<void> signal_map_changed; 
    // Emitted when the modification status changes (true/false)
    sigc::signal<void, bool> signal_modified_changed; 
    // Emitted when the layout needs to be refreshed (e.g. after undo/redo)
    sigc::signal<void> signal_layout_invalidated; 

private:
    std::shared_ptr<MindMap> m_Map;
    CommandManager m_commandManager;
    std::string m_currentFilename;
    bool m_modified = false;
    
    // Clipboard
    std::vector<std::shared_ptr<Node>> m_clipboard;
    std::vector<std::shared_ptr<Connection>> m_clipboardConnections;
};

#endif // MAPCONTROLLER_HPP
