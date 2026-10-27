#ifndef PRIORITYTASK_H
#define PRIORITYTASK_H

#include "TaskDecorator.h"

/**
 * @brief Concrete Decorator: gives a work item a priority (higher = more
 *        urgent). The value is also stored under the data key "priority", so
 *        rule based assignment and the TUI can read it through WorkItem.
 */
class PriorityTask : public TaskDecorator {
public:
    PriorityTask(WorkItem* task, int priority);
    PriorityTask(const PriorityTask& other);
    virtual ~PriorityTask();

    int getPriority() const;
    void setPriority(int priority);

    virtual WorkItem* clone() const;

private:
    int priority_;
};

#endif
