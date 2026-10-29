/**
 * @file StartCommand.cpp
 * @brief Implementation of the StartCommand class for transitioning work items to active execution.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

//../include/
#include "../include/StartCommand.h"

#include "../include/WorkItem.h"

/**
 * @brief Constructs a StartCommand targeting a specific work item.
 * @param[in] item Pointer to the WorkItem to be started.
 */
StartCommand::StartCommand(WorkItem* item) : WorkflowCommand(item) {}

/**
 * @brief Virtual destructor for StartCommand.
 */
StartCommand::~StartCommand() {}

/**
 * @brief Executes the start command on the target work item.
 * @details Captures a backup memento prior to starting the item to allow rollbacks.
 */
void StartCommand::execute() {
    saveBackup();
    item->start();
}

/**
 * @brief Reverts the start command.
 * @details Restores the work item to its prior state from the captured backup memento.
 */
void StartCommand::undo() {
    restoreBackup();
}

/**
 * @brief Generates a readable description of the start action.
 * @return std::string Summary string containing the target item name.
 */
std::string StartCommand::getDescription() const {
    return "Start '" + item->getName() + "'";
}