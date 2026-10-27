#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief End state of a task that was finished successfully (State pattern).
 *
 * No further actions are allowed. Any action falls back to TaskState,
 * which reports that it cannot be done.
 */
class CompletedState : public TaskState{

    public:
        /**
         * @brief Gets the name of this state.
         * @return "Completed".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new CompletedState.
         */
        TaskState* clone() const override;
        
};

#endif