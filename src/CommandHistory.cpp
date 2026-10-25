#include "../include/CommandHistory.h"

#include <stdexcept>

#include "../include/WorkflowCommand.h"

CommandHistory::CommandHistory() {}

CommandHistory::~CommandHistory() {
    for (auto& command : commands) {
        delete command;
    }
    commands.clear();
}

void CommandHistory::push(WorkflowCommand* command) {
    if (command == nullptr) {
        throw std::invalid_argument("CommandHistory::push: command is null");
    }
    commands.push_back(command);
}

WorkflowCommand* CommandHistory::pop() {
    if (commands.empty()) {
        return nullptr;
    }
    WorkflowCommand* command = commands.back();
    commands.pop_back();
    return command;
}

bool CommandHistory::isEmpty() const {
    return commands.empty();
}

int CommandHistory::size() const {
    return static_cast<int>(commands.size());
}