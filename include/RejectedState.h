#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief State of a task that was turned down (State pattern).
 *
 * From here the task can be made available again or cancelled.
 */
class RejectedState : public TaskState{

    public:

        /**
         * @brief Cancels the task and moves it to CancelledState.
         * @param task The task this state belongs to.
         */
        void cancel (Task* task) override;

        /**
         * @brief Clears the rejection reason and moves the task back to AvailableState.
         * @param task The task this state belongs to.
         */
        void makeAvailable (Task* task) override;

        /**
         * @brief Gets the name of this state.
         * @return "Rejected".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new RejectedState.
         */
        TaskState* clone() const override;

};

#endif