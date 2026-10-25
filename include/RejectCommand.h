#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include <string>

#include "WorkflowCommand.h"

class RejectCommand : public WorkflowCommand {
public:
    RejectCommand(WorkItem* item, const std::string& reason);
    ~RejectCommand();

    void execute();
    void undo();
    std::string getDescription() const;

private:
    std::string reason;
};

#endif