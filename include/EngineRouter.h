
#ifndef ENGINEROUTER_H
#define ENGINEROUTER_H

#include "WorkflowListener.h"
#include "WorkflowInstance.h"
#include "ApprovalHandler.h"

/**
 * @brief Routes workflow events to the appropriate handlers.
 */
class EngineRouter : public WorkflowListener {
private:
    /** @brief Workflow instance being managed. */
    WorkflowInstance* instance;

    /** @brief Approval handler chain. */
    ApprovalHandler* approvalChain;

public:
    /**
     * @brief Constructs an EngineRouter.
     * @param instance Pointer to the workflow instance.
     * @param approvalChain Pointer to the approval handler chain.
     */
    EngineRouter(WorkflowInstance* instance,
                 ApprovalHandler* approvalChain);

    /**
     * @brief Processes a workflow event.
     * @param event Pointer to the workflow event.
     */
    void update(WorkflowEvent* event) override;

    /** @brief Destroys the EngineRouter. */
    ~EngineRouter() override;
};

#endif // ENGINEROUTER_H
