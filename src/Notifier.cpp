#include "../include/Notifier.h"

/**
 * @brief Constructs a Notifier.
 */
Notifier::Notifier() {
}

/**
 * @brief Receives a workflow event.
 * @param event Pointer to the workflow event being published.
 */
void Notifier::update(WorkflowEvent* event) {
    if (event != nullptr) {
        // Add notification logic here (e.g. alert the people involved
        // when an item completes or times out).
    }
}

/**
 * @brief Destroys the Notifier.
 */
Notifier::~Notifier() {
}
