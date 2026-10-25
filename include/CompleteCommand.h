#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include <string>

#include "WorkflowCommand.h"

class CompleteCommand : public WorkflowCommand {
public:
    explicit CompleteCommand(WorkItem* item);
    ~CompleteCommand();

    void execute();
    void undo();
    std::string getDescription() const;
};

#endif