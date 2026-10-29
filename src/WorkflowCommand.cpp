/**
 * @file WorkflowCommand.cpp
 * @brief Implementation of the base WorkflowCommand class managing item memento snapshots.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#include "../include/WorkflowCommand.h"

#include <stdexcept>

#include "../include/TaskMemento.h"
#include "../include/WorkItem.h"

/**
 * @brief Constructs a WorkflowCommand targeting a specific WorkItem.
 * @param[in] item Pointer to the WorkItem this command will operate upon.
 * @throws std::invalid_argument If the provided WorkItem pointer is null.
 */
WorkflowCommand::WorkflowCommand(WorkItem* item) : item(item), backup(nullptr) {
    if (item == nullptr) {
        throw std::invalid_argument("WorkflowCommand: item is null");
    }
}

/**
 * @brief Virtual destructor that deallocates the backup memento snapshot.
 */
WorkflowCommand::~WorkflowCommand() {
    delete backup;
}

/**
 * @brief Captures and stores a memento snapshot of the associated work item.
 * @details Deallocates any existing backup before creating a new memento.
 */
void WorkflowCommand::saveBackup() {
    delete backup;
    backup = nullptr;
    backup = item->createMemento();
}

/**
 * @brief Restores the associated work item to its captured backup memento state.
 * @throws std::logic_error If no snapshot has been saved prior to restoration.
 */
void WorkflowCommand::restoreBackup() {
    if (backup == nullptr) {
        throw std::logic_error("undo: no snapshot was saved for '" + item->getName() + "'");
    }
    item->restoreMemento(backup);
}