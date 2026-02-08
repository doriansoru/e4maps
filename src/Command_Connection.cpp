#include "Command.hpp"
#include "Translation.hpp"

// AddConnectionCommand
AddConnectionCommand::AddConnectionCommand(std::shared_ptr<MindMap> m, std::shared_ptr<Node> f, std::shared_ptr<Node> t)
    : map(m), from(f), to(t), executed(false) {}

void AddConnectionCommand::execute() {
    if (!executed && map && from && to) {
        map->addConnection(from, to);
        executed = true;
    }
}

void AddConnectionCommand::undo() {
    if (executed && map && from && to) {
        map->removeConnection(from, to);
        executed = false;
    }
}

std::string AddConnectionCommand::getName() const {
    return _("Add Connection");
}

// RemoveConnectionCommand
RemoveConnectionCommand::RemoveConnectionCommand(std::shared_ptr<MindMap> m, std::shared_ptr<Node> f, std::shared_ptr<Node> t)
    : map(m), from(f), to(t), connectionCopy(f, t), executed(false) {
    // Try to find existing connection to copy its properties for undo
    if (map) {
        for (const auto& conn : map->connections) {
            if (conn->from == from && conn->to == to) {
                connectionCopy = *conn; // Store copy of data
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
        auto newConn = std::make_shared<Connection>(connectionCopy);
        map->connections.push_back(newConn);
        executed = false;
    }
}

std::string RemoveConnectionCommand::getName() const {
    return _("Remove Connection");
}
