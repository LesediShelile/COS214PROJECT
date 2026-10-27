#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief State of a task that is ready to be assigned (State pattern).
 *
 * From here the task can be assigned, rejected or cancelled.
 */
class AvailableState  : public TaskState {

    public: 

        /**
         * @brief Assigns a participant and moves the task to AssignedState.
         * @param task The task this state belongs to.
         * @param participant The participant to assign. If nullptr, the task stays Available.
         */
        void assign (Task* task, Participant* participant) override;

        /**
         * @brief Rejects the task, saves the reason and moves it to RejectedState.
         * @param task The task this state belongs to.
         * @param reason Why the task was rejected.
         */
        void reject (Task* task, std::string reason) override;

        /**
         * @brief Cancels the task and moves it to CancelledState.
         * @param task The task this state belongs to.
         */
        void cancel (Task* task) override;

        /**
         * @brief Gets the name of this state.
         * @return "Available".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new AvailableState.
         */
        TaskState* clone() const override;
};
#endif