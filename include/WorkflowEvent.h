
#ifndef WORKFLOWEVENT_H
#define WORKFLOWEVENT_H

#include <string>
#include "EventType.h"

/**
 * @brief Represents an event occurring in a workflow.
 */
class WorkflowEvent {
private:
    /** @brief Type of event. */
    EventType type;

    /** @brief ID of the event source. */
    std::string sourceID;

    /** @brief Timestamp of the event. */
    std::string timestamp;

    /** @brief Additional event details. */
    std::string details;

public:
    /**
     * @brief Constructs a workflow event.
     * @param type Type of event.
     * @param sourceID ID of the event source.
     * @param timestamp Timestamp of the event.
     * @param details Additional event details.
     */
    WorkflowEvent(EventType type, std::string sourceID,
                  std::string timestamp, std::string details);

    /** @brief Gets the event type. */
    EventType getType();

    /** @brief Gets the source ID. */
    std::string getSourceID();

    /** @brief Gets the event timestamp. */
    std::string getTimestamp();

    /** @brief Gets the event details. */
    std::string getDetails();

    /** @brief Destroys the workflow event. */
    ~WorkflowEvent();
};

#endif // WORKFLOWEVENT_H
