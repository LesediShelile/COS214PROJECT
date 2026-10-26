#ifndef RULEBASEDASSIGNMENT_H
#define RULEBASEDASSIGNMENT_H

#include <functional>
#include "AssignmentStrategy.h"

class RuleBasedAssignment : public AssignmentStrategy {
    public:

        RuleBasedAssignment(std::function<bool(Task*, Participant*)> rule);

        ~RuleBasedAssignment();

        // Picks the first participant for whom the rule returns true.
        Participant* selectParticipant(Task* task, std::vector<Participant*> participants) override;

        bool evaluateRule(Task* task, Participant* participant);

        void setRule(std::function<bool(Task*, Participant*)> rule);

    private:
    
        std::function<bool(Task*, Participant*)> rule;
};

#endif