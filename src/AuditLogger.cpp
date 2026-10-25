#include "../include/AuditLogger.h"

/**
 * @brief Constructs an AuditLogger.
 */
AuditLogger::AuditLogger() {
}

/**
 * @brief Processes a workflow event and records its details.
 * @param event Pointer to the workflow event being processed.
 */
void AuditLogger::update(WorkflowEvent* event) {
    if (event != nullptr) {
        sentMessages.push_back(event->getDetails());
    }
}

/**
 * @brief Retrieves all recorded messages.
 * @return A vector containing the recorded messages.
 */
std::vector<std::string> AuditLogger::getRecords() {
    return sentMessages;
}

/**
 * @brief Destroys the AuditLogger.
 */
AuditLogger::~AuditLogger() {
}
