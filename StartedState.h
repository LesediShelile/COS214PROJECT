#ifndef STARTSTATE_H
#define STARTSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"


/**
 * @brief State of a task that is being worked on (State pattern).
 *
 * From here the task can be completed, rejected or cancelled.
 */
class StartedState : public TaskState{

    public:

        /**
         * @brief Completes the task and moves it to CompletedState.
         * @param task The task this state belongs to.
         */
        void complete (Task* task);

        /**
         * @brief Rejects the task, saves the reason, clears the assignee and moves it to RejectedState.
         * @param task The task this state belongs to.
         * @param reason Why the task was rejected.
         */
        void reject (Task* task, std::string reason) override;

        /**
         * @brief Cancels the task and moves it to CancelledState.
         * @param task The task this state belongs to.
         */
        void cancel (Task* task);

        /**
         * @brief Gets the name of this state.
         * @return "Started".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new StartedState.
         */
        TaskState* clone() const override;
};

#endif