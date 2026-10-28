#include "TaskDecorator.h"
#include <stdexcept>

WorkItem* TaskDecorator::require(WorkItem* task) {
    if (!task) throw std::invalid_argument("TaskDecorator: null task");
    return task;
}

// require() is called inside each argument: C++ does not fix the evaluation
// order of constructor arguments, so a NULL check on one of them is not enough.
TaskDecorator::TaskDecorator(WorkItem* task)
    : WorkItem(require(task)->getItemID(), require(task)->getName()), task_(task) {}

TaskDecorator::TaskDecorator(const TaskDecorator& other)
    : WorkItem(other), task_(other.task_->clone()) {}

TaskDecorator::~TaskDecorator() { delete task_; }

std::string TaskDecorator::getItemID() const { return task_->getItemID(); }
std::string TaskDecorator::getName() const { return task_->getName(); }
void TaskDecorator::setData(const std::string& k, const std::string& v) { task_->setData(k, v); }
std::string TaskDecorator::getData(const std::string& k) const { return task_->getData(k); }
bool TaskDecorator::hasData(const std::string& k) const { return task_->hasData(k); }

void TaskDecorator::makeAvailable() { task_->makeAvailable(); }
void TaskDecorator::assign(Participant* p) { task_->assign(p); }
void TaskDecorator::start() { task_->start(); }
void TaskDecorator::complete() { task_->complete(); }
void TaskDecorator::reject(const std::string& r) { task_->reject(r); }
void TaskDecorator::cancel() { task_->cancel(); }

bool TaskDecorator::isFinished() const { return task_->isFinished(); }
bool TaskDecorator::isAvailable() const { return task_->isAvailable(); }
std::string TaskDecorator::getStateName() const { return task_->getStateName(); }

std::vector<WorkItem*> TaskDecorator::getChildren() const { return task_->getChildren(); }
void TaskDecorator::activateChildren() { task_->activateChildren(); }
void TaskDecorator::setPublisher(EventPublisher* p) { task_->setPublisher(p); }

TaskMemento* TaskDecorator::createMemento() { return task_->createMemento(); }
void TaskDecorator::restoreMemento(TaskMemento* m) { task_->restoreMemento(m); }

WorkItem* TaskDecorator::getWrapped() const { return task_; }

void TaskDecorator::setPublisher(EventPublisher* publisher) { task_->setPublisher(publisher); }