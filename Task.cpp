#include "Task.h"
#include "TaskState.h"
#include "CreatedState.h"
#include "TaskMemento.h"
#include "AssignmentStrategy.h"
#include "DepthFirstIterator.h"
#include "AvailableItemsIterator.h"

Task::Task(std::string itemID, std::string name, Role* role, AssignmentStrategy* strategy)
    : WorkItem(itemID, name),
      currentState(new CreatedState()),
      asignee(nullptr),
      requiredRole(role),
      strategy(strategy),
      deadline(0),
      publisher(nullptr) {}

Task::Task(const Task& other)
    : WorkItem(other),
      currentState(other.currentState->clone()),
      asignee(other.asignee),
      requiredRole(other.requiredRole),
      strategy(other.strategy),
      deadline(other.deadline),
      publisher(other.publisher) {}

Task::~Task() {
    delete currentState;
}

// ---------- State: Task just hands each action to its current state ----------

void Task::makeAvailable() {
    currentState->makeAvailable(this);
}

void Task::assign(Participant* participant) {
    currentState->assign(this, participant);
}

void Task::start() {
    currentState->start(this);
}

void Task::complete() {
    currentState->complete(this);
}

void Task::reject(std::string reason) {
    currentState->reject(this, reason);
}

void Task::cancel() {
    currentState->cancel(this);
}

// Called by the states to move the task into a new state
void Task::handleChange(TaskState* newState) {
    delete currentState;
    currentState = newState;
}

std::string Task::getStateName() {
    return currentState->getName();
}

bool Task::isFinished() {
    std::string state = currentState->getName();
    return state == "Completed" || state == "Cancelled";
}

bool Task::isAvailable() {
    return currentState->getName() == "Available";
}

// ---------- Prototype ----------

WorkItem* Task::clone() {
    return new Task(*this);
}

// ---------- Iterator ----------

WorkItemIterator* Task::createIterator() {
    return new DepthFirstIterator(this);
}

WorkItemIterator* Task::createAvailableIterator() {
    return new AvailableItemsIterator(this);
}

// ---------- Memento ----------

TaskMemento* Task::createMemento() {
    return new TaskMemento(currentState->clone(), asignee, data);
}

void Task::restoreMemento(TaskMemento* memento) {
    delete currentState;
    currentState = memento->state->clone();
    asignee = memento->assignee;
    data = memento->data;
}

// ---------- Getters / setters ----------

Participant* Task::getAssignee() {
    return asignee;
}

void Task::setAssignee(Participant* participant) {
    asignee = participant;
}

Role* Task::getRequiredRole() {
    return requiredRole;
}

// ---------- Strategy ----------

void Task::setAssignmentStrategy(AssignmentStrategy* strategy) {
    this->strategy = strategy;
}

void Task::autoAssign(std::vector<Participant*> candidates) {
    if (strategy == nullptr) {
        return;
    }
    Participant* chosen = strategy->selectParticipant(this, candidates);
    if (chosen != nullptr) {
        assign(chosen);
    }
}

void Task::setDeadline(time_t deadline) {
    this->deadline = deadline;
}

time_t Task::getDeadline() {
    return deadline;
}