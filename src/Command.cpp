#include "Command.hpp"
#include "Translation.hpp"
#include <iostream>

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

// ChangeThemeCommand
ChangeThemeCommand::ChangeThemeCommand(std::shared_ptr<MindMap> m, const Theme& oldT, const Theme& newT)
    : map(m), oldTheme(oldT), newTheme(newT), executed(false) {}

void ChangeThemeCommand::execute() {
    if (!executed && map) {
        map->theme = newTheme;
        executed = true;
    }
}

void ChangeThemeCommand::undo() {
    if (executed && map) {
        map->theme = oldTheme;
        executed = false;
    }
}

std::string ChangeThemeCommand::getName() const {
    return _("Change Theme");
}