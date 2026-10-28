#ifndef WORKFLOWCOMMAND_H
#define WORKFLOWCOMMAND_H

#include <string>

class WorkItem;
class TaskMemento;

/**
 * Command: an action on a work item, as an object that can be executed,
 * recorded and undone.
 *
 * Receiver: the WorkItem (not Task), so decorated tasks can be targeted too.
 * Before changing the item, a command saves a TaskMemento. undo() hands it back
 * so the item restores itself. The command owns its memento.
 */
class WorkflowCommand {
public:
    virtual ~WorkflowCommand();

    // Commands own a memento, so copying is disabled.
    WorkflowCommand(const WorkflowCommand&) = delete;
    WorkflowCommand& operator=(const WorkflowCommand&) = delete;

    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string getDescription() const = 0;

protected:
    explicit WorkflowCommand(WorkItem* item);

    // Take a fresh snapshot of the item (replaces any earlier one).
    // The snapshot is null if the item cannot be snapshotted (e.g. a Stage).
    void saveBackup();

    // Give the saved snapshot back to the item.
    // Throws std::logic_error if there is no snapshot to restore.
    void restoreBackup();

    WorkItem* item;
    TaskMemento* backup;
};

#endif