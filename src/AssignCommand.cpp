#include "../include/AssignCommand.h"

#include <stdexcept>

#include "../include/Participant.h"
#include "../include/WorkItem.h"

AssignCommand::AssignCommand(WorkItem* item, Participant* participant)
    : WorkflowCommand(item), participant(participant) {
    if (participant == nullptr) {
        throw std::invalid_argument("AssignCommand: participant is null");
    }
}

AssignCommand::~AssignCommand() {}

void AssignCommand::execute() {
    saveBackup();
    item->assign(participant);
}

void AssignCommand::undo() {
    restoreBackup();
}

std::string AssignCommand::getDescription() const {
    return "Assign '" + item->getName() + "' to " + participant->getName();
}