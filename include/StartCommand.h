#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

#include <string>

#include "WorkflowCommand.h"

/**
 * Command: starts a work item (Assigned -> Started).
 * Like the other commands it saves a memento first, so undo() puts the item
 * back to its previous state.
 */
class StartCommand : public WorkflowCommand {
public:
    explicit StartCommand(WorkItem* item);
    ~StartCommand();

    void execute();
    void undo();
    std::string getDescription() const;
};

#endif