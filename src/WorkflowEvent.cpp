#include "../include/WorkflowEvent.h"

/**
 * @brief Constructs a WorkflowEvent.
 * @param type The type of workflow event.
 * @param sourceID The ID of the event's source.
 * @param timestamp The time the event occurred.
 * @param details Additional details about the event.
 */
WorkflowEvent::WorkflowEvent(EventType type, std::string sourceID,
                             std::string timestamp, std::string details) {
    this->type = type;
    this->sourceID = sourceID;
    this->timestamp = timestamp;
    this->details = details;
}

/**
 * @brief Returns the event type.
 * @return The event type.
 */
EventType WorkflowEvent::getType() {
    return type;
}

/**
 * @brief Returns the source ID.
 * @return The event source ID.
 */
std::string WorkflowEvent::getSourceID() {
    return sourceID;
}

/**
 * @brief Returns the timestamp.
 * @return The event timestamp.
 */
std::string WorkflowEvent::getTimestamp() {
    return timestamp;
}

/**
 * @brief Returns the event details.
 * @return The event details.
 */
std::string WorkflowEvent::getDetails() {
    return details;
}

/**
 * @brief Destroys the WorkflowEvent.
 */
WorkflowEvent::~WorkflowEvent() {
}
