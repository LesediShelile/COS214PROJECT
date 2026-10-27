#ifndef WORKLOADASSIGNMENT_H
#define WORKLOADASSIGNMENT_H

#include "AssignmentStrategy.h"

/**
 * @brief Assigns a task to the least busy participant with the required role (Strategy pattern).
 */
class WorkloadAssignment : public AssignmentStrategy {

    public:

        /**
         * @brief Chooses the participant with the lowest workload who has the task's required role.
         * @param task The task that needs to be assigned.
         * @param participants The candidates to choose from.
         * @return The chosen participant, or nullptr if nobody is suitable.
         */
        Participant* selectParticipant(Task* task, std::vector<Participant*> participants) override;
};
#endif