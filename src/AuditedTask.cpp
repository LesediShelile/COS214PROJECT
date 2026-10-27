#include "AuditedTask.h"
#include <ctime>
#include "EventPublisher.h"
#include "WorkflowEvent.h"

namespace {
// The one event type this decorator publishes. If EventType has a better name
// for "something happened to a task", change it here only.
const EventType kTaskEvent = EventType::TASK_ACTION;

std::string now() {
    char buf[32];
    std::time_t t = std::time(0);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}
} // namespace

AuditedTask::AuditedTask(WorkItem* task, EventPublisher* publisher)
    : TaskDecorator(task), publisher_(publisher) {}

AuditedTask::AuditedTask(const AuditedTask& other)
    : TaskDecorator(other), publisher_(other.publisher_) {}

AuditedTask::~AuditedTask() {}

void AuditedTask::audit(const std::string& action) {
    if (!publisher_) return;
    WorkflowEvent event(kTaskEvent, getItemID(), now(),
                        "'" + getName() + "' " + action + " (now " + getStateName() + ")");
    publisher_->publish(&event);
}

void AuditedTask::makeAvailable() { TaskDecorator::makeAvailable(); audit("became available"); }
void AuditedTask::assign(Participant* p) { TaskDecorator::assign(p); audit("was assigned"); }
void AuditedTask::start() { TaskDecorator::start(); audit("was started"); }
void AuditedTask::complete() { TaskDecorator::complete(); audit("was completed"); }
void AuditedTask::reject(const std::string& reason) {
    TaskDecorator::reject(reason);
    audit("was rejected: " + reason);
}
void AuditedTask::cancel() { TaskDecorator::cancel(); audit("was cancelled"); }

WorkItem* AuditedTask::clone() const { return new AuditedTask(*this); }
