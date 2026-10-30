#ifndef Completed_H
#define Completed_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief End state of a task that was finished successfully (State pattern).
 *
 * No further actions are allowed. Any action falls back to TaskState,
 * which reports that it cannot be done.
 */
class Completed : public TaskState{

    public:
        /**
         * @brief Gets the name of this state.
         * @return "Completed".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new Completed.
         */
        TaskState* clone() const override;

        /**
         * @brief Completed is an end state.
         * @return true.
         */
        bool isFinal() const override { return true; }
        
};

#endif