#ifndef NOTIFIER_H
#define NOTIFIER_H
#include "WorkflowListener.h"

/**
 * @brief Handles notifications for workflow events (Observer pattern).
 *
 * A concrete WorkflowListener, so it can be attached to an EventPublisher
 * alongside the AuditLogger and MetricsCollector.
 */
class Notifier : public WorkflowListener {
public:
    /** @brief Constructs a Notifier. */
    Notifier();

    /**
     * @brief Receives a published workflow event.
     * @param event Pointer to the workflow event.
     */
    void update(WorkflowEvent* event) override;

    /** @brief Destroys the Notifier. */
    ~Notifier() override;
};

#endif // NOTIFIER_H
