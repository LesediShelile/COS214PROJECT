#ifndef WORKLOADASSIGNMENT_H
#define WORKLOADASSIGNMENT_H

#include "AssignmentStrategy.h"

class WorkloadAssignment : public AssignmentStrategy {

    public:

        Participant* selectParticipant(Task* task, std::vector<Participant*> participants) override;
};
#endif