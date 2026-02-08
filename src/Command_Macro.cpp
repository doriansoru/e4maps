#include "Command.hpp"

// MacroCommand
MacroCommand::MacroCommand(std::string cmdName) : name(cmdName) {}

void MacroCommand::addCommand(std::unique_ptr<Command> cmd) {
    commands.push_back(std::move(cmd));
}

void MacroCommand::execute() {
    for (auto& cmd : commands) {
        cmd->execute();
    }
}

void MacroCommand::undo() {
    // Undo in reverse order
    for (auto it = commands.rbegin(); it != commands.rend(); ++it) {
        (*it)->undo();
    }
}

std::string MacroCommand::getName() const {
    return name;
}
