#include "../include/EngineRouter.h"

/**
 * @brief Constructs an EngineRouter.
 * @param instance Pointer to the workflow instance.
 * @param approvalChain Pointer to the approval handler chain.
 */
EngineRouter::EngineRouter(WorkflowInstance* instance,
                           ApprovalHandler* approvalChain) {
    this->instance = instance;
    this->approvalChain = approvalChain;
}

/**
 * @brief Processes a workflow event.
 * @param event Pointer to the workflow event.
 */
void EngineRouter::update(WorkflowEvent* event) {
    if (event == nullptr) {
        return;
    }

    if (approvalChain != nullptr) {
        approvalChain->handleRequest(event);
    }
}

/**
 * @brief Destroys the EngineRouter.
 */
EngineRouter::~EngineRouter() {
}
