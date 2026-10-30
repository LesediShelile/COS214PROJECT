
#include "../include/EngineRouter.h"
#include "../include/ApprovalRequest.h"
#include "../include/EventType.h"

EngineRouter::EngineRouter(
    WorkflowInstance* instance,
    ApprovalHandler* approvalChain
) {
    this->instance = instance;
    this->approvalChain = approvalChain;
}

void EngineRouter::update(WorkflowEvent* event) {
    if (event == nullptr) {
        return;
    }

    // Handle approval timeout events.
    if (event->getType() == EventType::TIMEOUT) {
        ApprovalRequest* request = event->getApprovalRequest();

        if (request != nullptr && approvalChain != nullptr) {
            approvalChain->handle(request);
        }

        return;
    }

    // Ignore events if there is no active workflow instance.
    if (instance == nullptr) {
        return;
    }

    // Advance the workflow after an item completes.
    if (event->getType() == EventType::ITEM_COMPLETED) {
        instance->activateNext();
    }
}

EngineRouter::~EngineRouter() {}
