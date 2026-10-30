
#include "../include/WorkflowEvent.h"

WorkflowEvent::WorkflowEvent(
    EventType type,
    std::string sourceID,
    std::string timestamp,
    std::string details,
    ApprovalRequest* approvalRequest
) {
    this->type = type;
    this->sourceID = sourceID;
    this->timestamp = timestamp;
    this->details = details;
    this->approvalRequest = approvalRequest;
}

EventType WorkflowEvent::getType() {
    return type;
}

std::string WorkflowEvent::getSourceID() {
    return sourceID;
}

std::string WorkflowEvent::getTimestamp() {
    return timestamp;
}

std::string WorkflowEvent::getDetails() {
    return details;
}

ApprovalRequest* WorkflowEvent::getApprovalRequest() {
    return approvalRequest;
}

WorkflowEvent::~WorkflowEvent() {}
