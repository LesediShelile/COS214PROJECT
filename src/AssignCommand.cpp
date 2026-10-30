/**
 * @file AssignCommand.cpp
 * @brief Implementation of the AssignCommand class for delegating work items to participants.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#include "../include/AssignCommand.h"

#include <stdexcept>

#include "../include/Participant.h"
#include "../include/WorkItem.h"

/**
 * @brief Constructs an AssignCommand targeting a specific work item and participant.
 * @param[in] item Pointer to the target WorkItem to be assigned.
 * @param[in] participant Pointer to the Participant receiving the assignment.
 * @throws std::invalid_argument If the participant pointer is null.
 */
AssignCommand::AssignCommand(WorkItem* item, Participant* participant)
    : WorkflowCommand(item), participant(participant) {
    if (participant == nullptr) {
        throw std::invalid_argument("AssignCommand: participant is null");
    }
}

/**
 * @brief Virtual destructor for AssignCommand.
 */
AssignCommand::~AssignCommand() {}

/**
 * @brief Executes the assignment command.
 * @details Captures a backup memento of the target item's current state before
 * executing the assignment to support rollback.
 */
void AssignCommand::execute() {
    saveBackup();
    item->assign(participant);
}

/**
 * @brief Reverts the assignment command.
 * @details Restores the item state from the captured backup memento.
 */
void AssignCommand::undo() {
    restoreBackup();
}

/**
 * @brief Generates a readable description of the assignment action.
 * @return std::string A summary string detailing the item name and assignee.
 */
std::string AssignCommand::getDescription() const {
    return "Assign '" + item->getName() + "' to " + participant->getName();
}