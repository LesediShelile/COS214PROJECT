#include "WorkloadAssignment.h"
#include "Task.h"
#include "Participant.h"

Participant* WorkloadAssignment :: selectParticipant (Task* task, std::vector<Participant*> participants){
    
    Role* requiredRole = task -> getRequiredRole();

    Participant* best = nullptr;

    for (Participant* participant : participants) {

        if (participant == nullptr){
            continue;
        }

        if (requiredRole != nullptr && !participant -> hasRole(requiredRole)) {
            continue;
        }

        if (best == nullptr || participant -> getWorkload() < best -> getWorkload()) {
            best = participant;
        }
    }

    return best;
}