#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

/**
 * @brief End state of a task that was cancelled (State pattern).
 *
 * No further actions are allowed. Any action falls back to TaskState,
 * which reports that it cannot be done.
 */
class CancelledState : public TaskState{

    public:

        /**
         * @brief Gets the name of this state.
         * @return "Cancelled".
         */
        std::string getName() const override;

        /**
         * @brief Creates a copy of this state (Prototype pattern).
         * @return A new CancelledState.
         */
        TaskState* clone() const override;
};

#endif