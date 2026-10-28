#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include <string>

#include "WorkflowCommand.h"

class Participant;

class AssignCommand : public WorkflowCommand {
public:
    AssignCommand(WorkItem* item, Participant* participant);
    ~AssignCommand();

    void execute();
    void undo();
    std::string getDescription() const;

private:
    Participant* participant;
};

#endif
