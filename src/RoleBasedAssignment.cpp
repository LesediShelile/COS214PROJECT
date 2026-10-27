#include "RoleBasedAssignment.h"
#include "Task.h"
#include "Participant.h"

Participant* RoleBasedAssignment::selectParticipant(Task* task, std::vector<Participant*> participants) {

    for (Participant* participant : participants) {

        if (participant != nullptr && hasRequiredRole(participant, task)) {
            return participant;
        }
    }
    return nullptr;   // nobody has the required role
}

bool RoleBasedAssignment::hasRequiredRole(Participant* participant, Task* task) {

    Role* requiredRole = task->getRequiredRole();

    // A task with no required role can be done by anyone
    if (requiredRole == nullptr) {
        return true;
    }
    
    return participant->hasRole(requiredRole);
}