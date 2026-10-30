#ifndef Assigned_H
#define Assigned_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief State of a task that has a participant but has not been started (State pattern).
 *
 * From here the task can be started, rejected or cancelled.
 */
class Assigned  : public TaskState {

    public:

        /**
         * @brief Starts the task and moves it to StartedState.
         * @param start The task this state belongs to.
         */
        void start (Task* start) override;

        /**
         * @brief Rejects the task, saves the reason, clears the assignee and moves it to RejectedState.
         * @param task The task this state belongs to.
         * @param reason Why the task was rejected.
         */
        void reject(Task* task, std::string reason) override;

        /**
         * @brief Cancels the task and moves it to Cancelled.
         * @param task The task this state belongs to.
         */
        void cancel (Task* task)  override;

        /**
         * @brief Gets the name of this state.
         * @return "Assigned".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new Assigned.
         */
        TaskState* clone() const override;
};
#endif