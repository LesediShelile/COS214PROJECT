#include "../include/EventPublisher.h"

/**
 * @brief Constructs an EventPublisher.
 */
EventPublisher::EventPublisher() {
}

/**
 * @brief Registers a listener to receive workflow events.
 * @param listener Pointer to the listener to register.
 */
void EventPublisher::attach(WorkflowListener* listener) {
    if (listener != nullptr) {
        listerList.push_back(listener);
    }
}

/**
 * @brief Removes a listener from the publisher.
 * @param listener Pointer to the listener to remove.
 */
void EventPublisher::detach(WorkflowListener* listener) {
    for (std::vector<WorkflowListener*>::iterator it = listerList.begin();
         it != listerList.end(); ++it) {
        if (*it == listener) {
            listerList.erase(it);
            break;
        }
    }
}

/**
 * @brief Publishes a workflow event to all registered listeners.
 * @param event Pointer to the workflow event being published.
 */
void EventPublisher::publish(WorkflowEvent* event) {
    if (event == nullptr) {
        return;
    }

    for (std::vector<WorkflowListener*>::iterator it = listerList.begin();
         it != listerList.end(); ++it) {
        if (*it != nullptr) {
            (*it)->update(event);
        }
    }
}

/**
 * @brief Destroys the EventPublisher.
 */
EventPublisher::~EventPublisher() {
    listerList.clear();
}
