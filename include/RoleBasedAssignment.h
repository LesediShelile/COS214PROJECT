#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include "AssignmentStrategy.h"

/**
 * @brief Assigns a task to the first participant with the required role (Strategy pattern).
 */
class RoleBasedAssignment :public AssignmentStrategy {
public: 
    /**
     * @brief Chooses the first participant who has the task's required role.
     * @param task The task that needs to be assigned.
     * @param participants The candidates to choose from.
     * @return The chosen participant, or nullptr if nobody has the role.
     */
    Participant* selectParticipant (Task* task, std::vector<Participant*> participants) override;

    /**
     * @brief Checks whether a participant has the role the task needs.
     * @param participant The participant to check.
     * @param task The task whose required role is checked.
     * @return True if the participant has the role, or if the task needs no role.
     */
    bool hasRequiredRole (Participant* participant, Task* task);

};

#endif