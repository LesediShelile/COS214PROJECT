//../include/
#include "../include/RejectCommand.h"

#include "../include/WorkItem.h"

RejectCommand::RejectCommand(WorkItem* item, const std::string& reason)
    : WorkflowCommand(item), reason(reason) {}

RejectCommand::~RejectCommand() {}

void RejectCommand::execute() {
    saveBackup();
    item->reject(reason);
}

void RejectCommand::undo() {
    restoreBackup();
}

std::string RejectCommand::getDescription() const {
    return "Reject '" + item->getName() + "': " + reason;
}