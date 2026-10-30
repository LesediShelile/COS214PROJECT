/**
 * @file CompleteCommand.cpp
 * @brief Implementation of the CompleteCommand class for finishing work items.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

//../include/
#include "../include/CompleteCommand.h"

#include "../include/WorkItem.h"

/**
 * @brief Constructs a CompleteCommand targeting a specific work item.
 * @param[in] item Pointer to the WorkItem to be completed.
 */
CompleteCommand::CompleteCommand(WorkItem* item) : WorkflowCommand(item) {}

/**
 * @brief Virtual destructor for CompleteCommand.
 */
CompleteCommand::~CompleteCommand() {}

/**
 * @brief Executes the completion command on the target work item.
 * @details Captures a backup memento before transitioning the item state to support undo operations.
 */
void CompleteCommand::execute() {
    saveBackup();
    item->complete();
}

/**
 * @brief Reverts the completion command.
 * @details Restores the work item to its prior state using the saved backup memento.
 */
void CompleteCommand::undo() {
    restoreBackup();
}

/**
 * @brief Generates a readable description of the completion command.
 * @return std::string A summary description containing the target item name.
 */
std::string CompleteCommand::getDescription() const {
    return "Complete '" + item->getName() + "'";
}