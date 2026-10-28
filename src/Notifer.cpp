#include "../include/Notifier.h"

/**
 * @brief Constructs a Notifier.
 */
Notifier::Notifier() {
}

/**
 * @brief Updates the notifier with the given task state.
 * @param state Pointer to the task state being updated.
 */
void Notifier::update(TaskState* state) {
    if (state != nullptr) {
        // Add notification logic here when the required
        // TaskState interface is available.
    }
}

/**
 * @brief Destroys the Notifier.
 */
Notifier::~Notifier() {
}
