#include "Task.h"
#include <stdexcept>
#include "TaskState.h"
#include "CreatedState.h"
#include "TaskMemento.h"
#include "AssignmentStrategy.h"

Task::Task(const std::string& itemID, const std::string& name,
           Role* requiredRole, AssignmentStrategy* strategy)
    : WorkItem(itemID, name), currentState_(new Created()),
      assignee_(0), requiredRole_(requiredRole), strategy_(strategy),
      deadline_(0), publisher_(0) {}

Task::Task(const Task& other)
    : WorkItem(other), currentState_(other.currentState_->clone()),
      assignee_(other.assignee_), requiredRole_(other.requiredRole_),
      strategy_(other.strategy_), deadline_(other.deadline_),
      publisher_(other.publisher_) {}

Task::~Task() { delete currentState_; }

void Task::makeAvailable() { currentState_->makeAvailable(this); }
void Task::assign(Participant* p) { currentState_->assign(this, p); }
void Task::start() { currentState_->start(this); }
void Task::complete() { currentState_->complete(this); }
void Task::reject(const std::string& reason) { currentState_->reject(this, reason); }
void Task::cancel() { currentState_->cancel(this); }

bool Task::isFinished() const { return currentState_->isFinal(); }
bool Task::isAvailable() const { return currentState_->isAvailable(); }
std::string Task::getStateName() const { return currentState_->getName(); }

void Task::handleChange(TaskState* newState) {
    if (!newState) throw std::invalid_argument("Task::handleChange: null state");
    if (newState == currentState_) return;
    delete currentState_;
    currentState_ = newState;
}

WorkItem* Task::clone() const { return new Task(*this); }

TaskMemento* Task::createMemento() {
    return new TaskMemento(currentState_->clone(), assignee_, data_);
}

void Task::restoreMemento(TaskMemento* m) {
    if (!m) throw std::invalid_argument("Task::restoreMemento: null memento");
    TaskState* restored = m->state->clone();
    delete currentState_;
    currentState_ = restored;
    assignee_ = m->assignee;
    data_ = m->data;
}

Participant* Task::getAssignee() const { return assignee_; }
void Task::setAssignee(Participant* p) { assignee_ = p; }
Role* Task::getRequiredRole() const { return requiredRole_; }

void Task::setAssignmentStrategy(AssignmentStrategy* s) { strategy_ = s; }

void Task::autoAssign(const std::vector<Participant*>& candidates) {
    if (!strategy_) throw std::logic_error("Task::autoAssign: no assignment strategy");
    Participant* chosen = strategy_->selectParticipant(this, candidates);
    if (!chosen) throw std::runtime_error("Task::autoAssign: no eligible participant");
    assign(chosen);
}

void Task::setDeadline(std::time_t d) { deadline_ = d; }
std::time_t Task::getDeadline() const { return deadline_; }
void Task::setPublisher(EventPublisher* p) { publisher_ = p; }
EventPublisher* Task::getPublisher() const { return publisher_; }
