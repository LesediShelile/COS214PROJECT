#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief Starting state of every new task (State pattern).
 *
 * From here the task can be made available or cancelled.
 */
class CreatedState : public TaskState {

    public:

        /**
         * @brief Makes the task ready to be assigned and moves it to AvailableState.
         * @param task The task this state belongs to.
         */
        void makeAvailable (Task* task) override;

        /**
         * @brief Gets the name of this state.
         * @return "Created".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new CreatedState.
         */
        TaskState* clone() const override;

        /**
         * @brief Cancels the task and moves it to CancelledState.
         * @param task The task this state belongs to.
         */
        void cancel (Task* task) override;
        
};
#endif