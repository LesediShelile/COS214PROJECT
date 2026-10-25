
#ifndef NOTIFIER_H
#define NOTIFIER_H

#include "TaskState.h"

/**
 * @brief Handles notifications related to task states.
 */
class Notifier {
public:
    /** @brief Constructs a Notifier. */
    Notifier();

    /**
     * @brief Updates the notifier with a task state.
     * @param state Pointer to the task state.
     */
    void update(TaskState* state);

    /** @brief Destroys the Notifier. */
    ~Notifier();
};

#endif // NOTIFIER_H
