//../include/
#include "../include/StartCommand.h"

#include "../include/WorkItem.h"

StartCommand::StartCommand(WorkItem* item) : WorkflowCommand(item) {}

StartCommand::~StartCommand() {}

void StartCommand::execute() {
    saveBackup();
    item->start();
}

void StartCommand::undo() {
    restoreBackup();
}

std::string StartCommand::getDescription() const {
    return "Start '" + item->getName() + "'";
}