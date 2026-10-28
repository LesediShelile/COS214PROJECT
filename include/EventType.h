#ifndef EVENTTYPE_H
#define EVENTTYPE_H

/**
 * @brief Kinds of events published through the Observer pattern.
 *
 * Every WorkflowEvent carries one of these, so listeners (audit logger,
 * metrics collector, notifier, engine router) can decide what to react to.
 */
enum class EventType {
    /** An item moved to a new lifecycle state (details = new state name). */
    ITEM_STATE_CHANGED,

    /** An item reached the Completed state. The engine uses this to activate
     *  the next item(s) in the workflow. */
    ITEM_COMPLETED,

    /** Something was done to a task and recorded by AuditedTask. */
    TASK_ACTION,

    /** An approval request ran out of time before it was answered. */
    TIMEOUT,

    /** The whole workflow instance finished (details = outcome). */
    WORKFLOW_FINISHED
};

#endif // EVENTTYPE_H
