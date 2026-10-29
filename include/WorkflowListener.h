
#ifndef WORKFLOWLISTENER_H
#define WORKFLOWLISTENER_H

#include "WorkflowEvent.h"

/**
 * @brief Interface for objects that receive workflow events.
 */
class WorkflowListener {
public:
    /** @brief Constructs a WorkflowListener. */
    WorkflowListener();

    /**
     * @brief Handles a published workflow event.
     * @param event Pointer to the workflow event.
     */
    virtual void update(WorkflowEvent* event) = 0;

    /** @brief Destroys the WorkflowListener. */
    virtual ~WorkflowListener();
};

#endif // WORKFLOWLISTENER_H
