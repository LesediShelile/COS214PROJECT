#include "PriorityTask.h"
#include <string>

PriorityTask::PriorityTask(WorkItem* task, int priority)
    : TaskDecorator(task), priority_(priority) {
    setData("priority", std::to_string(priority_));
}

PriorityTask::PriorityTask(const PriorityTask& other)
    : TaskDecorator(other), priority_(other.priority_) {}

PriorityTask::~PriorityTask() {}

int PriorityTask::getPriority() const { return priority_; }

void PriorityTask::setPriority(int priority) {
    priority_ = priority;
    setData("priority", std::to_string(priority_));
}

WorkItem* PriorityTask::clone() const { return new PriorityTask(*this); }
