#include "../include/CommandInvoker.h"

#include <memory>
#include <stdexcept>

#include "../include/WorkflowCommand.h"

CommandInvoker::CommandInvoker() {}

CommandInvoker::~CommandInvoker() {}

void CommandInvoker::executeCommand(WorkflowCommand* command) {
    if (command == nullptr) {
        throw std::invalid_argument("executeCommand: command is null");
    }
    std::unique_ptr<WorkflowCommand> owned(command);
    owned->execute();            // if this throws, 'owned' deletes the command
    history.push(owned.get());   // history now owns it...
    owned.release();             // ...so stop owning it here - only 1 owner
}

bool CommandInvoker::undoLast() {
    if (history.isEmpty()) {
        return false;
    }
    std::unique_ptr<WorkflowCommand> command(history.pop());
    command->undo();
    return true;
}

bool CommandInvoker::canUndo() const {
    return !history.isEmpty();
}

CommandHistory* CommandInvoker::getHistory() {
    return &history;
}