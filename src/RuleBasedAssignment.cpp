#include "RuleBasedAssignment.h"

RuleBasedAssignment::RuleBasedAssignment(std::function<bool(Task*, Participant*)> rule) : rule(rule) {}

RuleBasedAssignment::~RuleBasedAssignment() {}

Participant* RuleBasedAssignment::selectParticipant(Task* task, std::vector<Participant*> participants) {

    for (Participant* participant : participants) {

        if (participant != nullptr && evaluateRule(task, participant)) {
            return participant;
        }
    }

    return nullptr;   // nobody passed the rule
}

bool RuleBasedAssignment::evaluateRule(Task* task, Participant* participant) {
    // No rule set means nobody can be chosen
    if (!rule) {
        return false;
    }

    return rule(task, participant);
}

void RuleBasedAssignment::setRule(std::function<bool(Task*, Participant*)> rule) {
    
    this->rule = rule;
}