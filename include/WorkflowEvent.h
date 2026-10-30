
#ifndef WORKFLOWEVENT_H
#define WORKFLOWEVENT_H

#include <string>
#include "EventType.h"

class ApprovalRequest;

class WorkflowEvent {
private:
    EventType type;
    std::string sourceID;
    std::string timestamp;
    std::string details;

    // Non-owning pointer: the event does not manage the request.
    ApprovalRequest* approvalRequest;

public:
    WorkflowEvent(
        EventType type,
        std::string sourceID,
        std::string timestamp,
        std::string details,
        ApprovalRequest* approvalRequest = nullptr
    );

    EventType getType();
    std::string getSourceID();
    std::string getTimestamp();
    std::string getDetails();

    ApprovalRequest* getApprovalRequest();

    ~WorkflowEvent();
};

#endif
