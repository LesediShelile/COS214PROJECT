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

void Created::makeAvailable(Task* t) { t->handleChange(new Available()); }
void Created::cancel(Task* t) { t->handleChange(new Cancelled()); }

void Available::assign(Task* t, Participant* p) { t->setAssignee(p); t->handleChange(new Assigned()); }
void Available::cancel(Task* t) { t->handleChange(new Cancelled()); }

void Assigned::start(Task* t) { t->handleChange(new StartedState()); }
void Assigned::reject(Task* t, const std::string&) { t->setAssignee(0); t->handleChange(new RejectedState()); }
void Assigned::cancel(Task* t) { t->handleChange(new Cancelled()); }

void StartedState::complete(Task* t) { t->handleChange(new Completed()); }
void StartedState::reject(Task* t, const std::string&) { t->setAssignee(0); t->handleChange(new RejectedState()); }
void StartedState::cancel(Task* t) { t->handleChange(new Cancelled()); }

void RejectedState::makeAvailable(Task* t) { t->handleChange(new Available()); }
void RejectedState::cancel(Task* t) { t->handleChange(new Cancelled()); }
