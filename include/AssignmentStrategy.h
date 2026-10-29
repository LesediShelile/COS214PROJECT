#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <iostream>
#include <string>
#include <vector>

class Task;
class Participant;

/**
 * @brief Abstract strategy for choosing who a task is assigned to (Strategy pattern).
 *
 * Concrete strategies decide how a participant is picked, for example
 * by role, by workload or by a custom rule.
 */
class AssignmentStrategy {

    public:

        /** @brief Virtual destructor so concrete strategies are cleaned up correctly. */
        virtual ~AssignmentStrategy();

        /**
         * @brief Chooses a participant for the task.
         * @param task The task that needs to be assigned.
         * @param participants The candidates to choose from.
         * @return The chosen participant, or nullptr if nobody is suitable.
         */
        virtual Participant* selectParticipant (Task* task, std::vector<Participant*> participants) = 0;

};

#endif