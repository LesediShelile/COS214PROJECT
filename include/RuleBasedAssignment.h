#ifndef RULEBASEDASSIGNMENT_H
#define RULEBASEDASSIGNMENT_H

#include <functional>
#include "AssignmentStrategy.h"

/**
 * @brief Assigns a task using a custom rule (Strategy pattern).
 *
 * The rule is any function that takes a task and a participant
 * and returns true if that participant may be chosen.
 */
class RuleBasedAssignment : public AssignmentStrategy {
    public:

        /**
         * @brief Creates the strategy with a rule.
         * @param rule Returns true if the participant may be chosen for the task.
         */
        RuleBasedAssignment(std::function<bool(Task*, Participant*)> rule);

        /** @brief Destructor. */
        ~RuleBasedAssignment();

        /**
         * @brief Chooses the first participant who passes the rule.
         * @param task The task that needs to be assigned.
         * @param participants The candidates to choose from.
         * @return The chosen participant, or nullptr if nobody passes.
         */
        // Picks the first participant for whom the rule returns true.
        Participant* selectParticipant(Task* task, std::vector<Participant*> participants) override;

        /**
         * @brief Runs the rule for one participant.
         * @param task The task that needs to be assigned.
         * @param participant The participant to check.
         * @return The rule's result, or false if no rule is set.
         */
        bool evaluateRule(Task* task, Participant* participant);

        /**
         * @brief Replaces the rule.
         * @param rule The new rule to use.
         */
        void setRule(std::function<bool(Task*, Participant*)> rule);

    private:
    
        /** @brief The rule used to decide who may be chosen. */
        std::function<bool(Task*, Participant*)> rule;
};

#endif