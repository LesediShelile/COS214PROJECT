#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include "AssignmentStrategy.h"

class RoleBasedAssignment :public AssignmentStrategy {

    Participant* selectParticipant (Task* task, std::vector<Participant*> participants) override;

    bool hasRequiredRole (Participant* participant, Task* task);

};

#endif