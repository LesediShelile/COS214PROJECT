/**
 * @file WorkflowCommand.h
 * @brief Header file declaring the abstract WorkflowCommand base class.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef WORKFLOWCOMMAND_H
#define WORKFLOWCOMMAND_H

#include <string>

class WorkItem;
class TaskMemento;

/**
 * @class WorkflowCommand
 * @brief Abstract base class representing an undoable action on a work item.
 * 
 * Implements the Command design pattern. Represents an action on a WorkItem
 * as an object that can be executed, recorded in history, and undone.
 * Acts on the WorkItem abstraction (rather than concrete Task directly) so
 * that decorated items can also be targeted. Before modifying the item,
 * a command captures a TaskMemento snapshot, which is restored during undo().
 * The command owns the lifecycle of its captured memento.
 * 
 * @see WorkItem
 * @see TaskMemento
 */
class WorkflowCommand {
public:
    /**
     * @brief Virtual destructor ensuring clean polymorphic teardown and memento cleanup.
     */
    virtual ~WorkflowCommand();

    /**
     * @brief Deleted copy constructor to prevent multiple ownership of memento pointers.
     */
    WorkflowCommand(const WorkflowCommand&) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent multiple ownership of memento pointers.
     */
    WorkflowCommand& operator=(const WorkflowCommand&) = delete;

    /**
     * @brief Executes the specific workflow operation.
     */
    virtual void execute() = 0;

    /**
     * @brief Reverts the workflow operation by restoring the item's prior state.
     */
    virtual void undo() = 0;

    /**
     * @brief Generates a readable description of the command.
     * @return std::string Description summarizing the action.
     */
    virtual std::string getDescription() const = 0;

protected:
    /**
     * @brief Protected constructor initializing the receiver WorkItem.
     * @param[in] item Pointer to the target WorkItem receiver.
     */
    explicit WorkflowCommand(WorkItem* item);

    /**
     * @brief Captures a fresh memento snapshot from the target work item.
     * @details Replaces and deletes any previously captured snapshot. If the item
     * cannot be snapshotted (e.g., a Stage), the backup pointer remains null.
     */
    void saveBackup();

    /**
     * @brief Restores the target work item using the saved snapshot.
     * @throws std::logic_error If no snapshot has been captured prior to calling restore.
     */
    void restoreBackup();

    WorkItem* item;       ///< Pointer to the receiver WorkItem targeted by the command.
    TaskMemento* backup;  ///< Owned pointer to the captured TaskMemento snapshot.
};

#endif