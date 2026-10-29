#include "TaskState.h"
#include <stdexcept>
#include "Task.h"

void TaskState::invalid(const char* op) const {
    throw std::logic_error(std::string("Invalid transition: ") + op +
                           " not allowed in state " + getName());
}
void TaskState::makeAvailable(Task*) { invalid("makeAvailable"); }
void TaskState::assign(Task*, Participant*) { invalid("assign"); }
void TaskState::start(Task*) { invalid("start"); }
void TaskState::complete(Task*) { invalid("complete"); }
void TaskState::reject(Task*, const std::string&) { invalid("reject"); }
void TaskState::cancel(Task*) { invalid("cancel"); }

void CreatedState::makeAvailable(Task* t) { t->handleChange(new AvailableState()); }
void CreatedState::cancel(Task* t) { t->handleChange(new CancelledState()); }

void AvailableState::assign(Task* t, Participant* p) { t->setAssignee(p); t->handleChange(new AssignedState()); }
void AvailableState::cancel(Task* t) { t->handleChange(new CancelledState()); }

void AssignedState::start(Task* t) { t->handleChange(new StartedState()); }
void AssignedState::reject(Task* t, const std::string&) { t->setAssignee(0); t->handleChange(new RejectedState()); }
void AssignedState::cancel(Task* t) { t->handleChange(new CancelledState()); }

void StartedState::complete(Task* t) { t->handleChange(new CompletedState()); }
void StartedState::reject(Task* t, const std::string&) { t->setAssignee(0); t->handleChange(new RejectedState()); }
void StartedState::cancel(Task* t) { t->handleChange(new CancelledState()); }

void RejectedState::makeAvailable(Task* t) { t->handleChange(new AvailableState()); }
void RejectedState::cancel(Task* t) { t->handleChange(new CancelledState()); }
