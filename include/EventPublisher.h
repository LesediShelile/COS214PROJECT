
#ifndef EVENTPUBLISHER_H
#define EVENTPUBLISHER_H

#include <vector>
#include "WorkflowListener.h"
#include "WorkflowEvent.h"

/**
 * @brief Publishes workflow events to registered listeners.
 */
class EventPublisher {
private:
    /** @brief Registered workflow listeners. */
    std::vector<WorkflowListener*> listerList;

public:
    /** @brief Constructs an EventPublisher. */
    EventPublisher();

    /**
     * @brief Registers a listener.
     * @param listener Pointer to the listener to register.
     */
    void attach(WorkflowListener* listener);

    /**
     * @brief Unregisters a listener.
     * @param listener Pointer to the listener to remove.
     */
    void detach(WorkflowListener* listener);

    /**
     * @brief Notifies registered listeners of an event.
     * @param event Pointer to the event being published.
     */
    void publish(WorkflowEvent* event);

    /** @brief Destroys the EventPublisher. */
    ~EventPublisher();
};

#endif // EVENTPUBLISHER_H
