#ifndef AUDITEDTASK_H
#define AUDITEDTASK_H

#include <string>
#include "TaskDecorator.h"

class EventPublisher;

/**
 * @brief Concrete Decorator: publishes a WorkflowEvent after every successful
 *        lifecycle operation on the wrapped item, so listeners (AuditLogger,
 *        MetricsCollector, Notifier) see it without the task knowing them.
 *
 * If the wrapped operation throws (an invalid transition, say), nothing is
 * published, so the audit trail only records what actually happened.
 * The publisher is shared, not owned. A NULL publisher makes this a no-op.
 */
class AuditedTask : public TaskDecorator {
public:
    AuditedTask(WorkItem* task, EventPublisher* publisher);
    AuditedTask(const AuditedTask& other);
    virtual ~AuditedTask();

    virtual void makeAvailable();
    virtual void assign(Participant* participant);
    virtual void start();
    virtual void complete();
    virtual void reject(const std::string& reason);
    virtual void cancel();

    /** @brief Replaces this decorator's publisher and the wrapped item's. */
    virtual void setPublisher(EventPublisher* publisher);

    virtual WorkItem* clone() const;

private:
    EventPublisher* publisher_;
    /** @brief Build and publish one event describing @p action. */
    void audit(const std::string& action);
};

#endif
