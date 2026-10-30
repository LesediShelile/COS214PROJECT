#ifndef Created_H
#define Created_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief Starting state of every new task (State pattern).
 *
 * From here the task can be made available or cancelled.
 */
class Created : public TaskState {

    public:

        /**
         * @brief Makes the task ready to be assigned and moves it to Available.
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
         * @return A new Created.
         */
        TaskState* clone() const override;

        /**
         * @brief Cancels the task and moves it to Cancelled.
         * @param task The task this state belongs to.
         */
        void cancel (Task* task) override;
        
};
#endif