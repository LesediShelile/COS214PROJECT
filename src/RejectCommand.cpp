/**
 * @file RejectCommand.cpp
 * @brief Implementation of the RejectCommand class for rejecting work items with a specified reason.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

//../include/
#include "../include/RejectCommand.h"

#include "../include/WorkItem.h"

/**
 * @brief Constructs a RejectCommand targeting a specific work item and supplying a rejection reason.
 * @param[in] item Pointer to the WorkItem to be rejected.
 * @param[in] reason Explanation string detailing why the work item is rejected.
 */
RejectCommand::RejectCommand(WorkItem* item, const std::string& reason)
    : WorkflowCommand(item), reason(reason) {}

/**
 * @brief Virtual destructor for RejectCommand.
 */
RejectCommand::~RejectCommand() {}

/**
 * @brief Executes the rejection command on the target work item.
 * @details Captures a backup memento before transitioning the item state to support undo operations.
 */
void RejectCommand::execute() {
    saveBackup();
    item->reject(reason);
}

/**
 * @brief Reverts the rejection command.
 * @details Restores the work item to its prior state using the saved backup memento.
 */
void RejectCommand::undo() {
    restoreBackup();
}

/**
 * @brief Generates a readable description of the rejection command.
 * @return std::string A summary description containing the target item name and rejection reason.
 */
std::string RejectCommand::getDescription() const {
    return "Reject '" + item->getName() + "': " + reason;
}