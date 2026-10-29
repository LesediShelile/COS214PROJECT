#include "Participant.h"
#include "Role.h"

Participant::Participant(std::string participantID, std::string name, Role* role) : participantID(participantID), name(name), role(role), workload(0) {}

std::string Participant::getParticipantID() const {

    return participantID;
    
}

std::string Participant::getName() const {

    return name;

}

Role* Participant::getRole() const {

    return role;

}

int Participant::getWorkload() const {

    return workload;

}

void Participant::incrementWorkload() {

    workload++;

}

void Participant::decrementWorkload() {

    // Workload can never be negative
    if (workload > 0) {
        workload--;
    }

}

bool Participant::hasRole(Role* role) const {

    return this->role != nullptr && this->role->equals(role);

}