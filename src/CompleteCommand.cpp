//../include/
#include "../include/CompleteCommand.h"

#include "../include/WorkItem.h"

CompleteCommand::CompleteCommand(WorkItem* item) : WorkflowCommand(item) {}

CompleteCommand::~CompleteCommand() {}

void CompleteCommand::execute() {
    saveBackup();
    item->complete();
}

void CompleteCommand::undo() {
    restoreBackup();
}

std::string CompleteCommand::getDescription() const {
    return "Complete '" + item->getName() + "'";
}