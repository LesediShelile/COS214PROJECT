#include "../include/WorkflowCommand.h"

#include <stdexcept>

#include "../include/TaskMemento.h"
#include "../include/WorkItem.h"

WorkflowCommand::WorkflowCommand(WorkItem* item) : item(item), backup(nullptr) {
    if (item == nullptr) {
        throw std::invalid_argument("WorkflowCommand: item is null");
    }
}

WorkflowCommand::~WorkflowCommand() {
    delete backup;
}

void WorkflowCommand::saveBackup() {
    delete backup;
    backup = nullptr;
    backup = item->createMemento();
}

void WorkflowCommand::restoreBackup() {
    if (backup == nullptr) {
        throw std::logic_error("undo: no snapshot was saved for '" + item->getName() + "'");
    }
    item->restoreMemento(backup);
}