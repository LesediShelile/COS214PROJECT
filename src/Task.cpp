#include "../include/Task.h"
#include "../include/WorkItem.h"
#include "../include/WorkItemIterator.h"
#include "../include/TaskMemento.h"

/**
 * @brief Constructs a Task.
 */
Task::Task(std::string itemID, std::string name,
           Role* role, AssignmentStrategy* strategy)
    : WorkItem(itemID, name),
      currentState(0),
      asignee(0),
      requiredRole(role),
      strategy(strategy),
      deadline(0),
      publisher(0) {
}

/**
 * @brief Destroys the Task.
 */
Task::~Task() {
    delete currentState;
}

/**
 * @brief Makes the task available for assignment.
 */
void Task::makeAvailable() {
    if (currentState != 0) {
        currentState->makeAvailable(this);
    }
}

/**
 * @brief Assigns a participant to the task.
 */
void Task::assign(Participant* participant) {
    asignee = participant;
}

/**
 * @brief Changes the task's state.
 */
void Task::handleChange(TaskState* newState) {
    if (currentState != newState) {
        delete currentState;
        currentState = newState;
    }
}

/**
 * @brief Starts the task.
 */
void Task::start() {
    if (currentState != 0) {
        currentState->start(this);
    }
}

/**
 * @brief Marks the task as complete.
 */
void Task::complete() {
    if (currentState != 0) {
        currentState->complete(this);
    }
}

/**
 * @brief Rejects the task.
 */
void Task::reject(std::string* reason) {
    if (currentState != 0) {
        currentState->reject(this, *reason);
    }
}

/**
 * @brief Cancels the task.
 */
void Task::cancel() {
    if (currentState != 0) {
        currentState->cancel(this);
    }
}

/**
 * @brief Checks whether the task has finished.
 */
bool Task::isFinished() {
    return currentState != 0 && currentState->isFinished();
}

/**
 * @brief Checks whether the task is available.
 */
bool Task::isAvailable() {
    return currentState != 0 && currentState->isAvailable();
}

/**
 * @brief Returns the current state name.
 */
std::string Task::getStateName() {
    if (currentState != 0) {
        return currentState->getName();
    }

    return "Created";
}

/**
 * @brief Creates a copy of the task.
 */
WorkItem* Task::clone() {
    return new Task(*this);
}

/**
 * @brief Creates an iterator for the task.
 */
WorkItemIterator* Task::createIterator() {
    return 0;
}

/**
 * @brief Creates a memento of the task.
 */
TaskMemento* Task::createMemento() {
    return 0;
}

/**
 * @brief Restores the task from a memento.
 */
void Task::restoreMemento(TaskMemento* memento) {
    (void)memento;
}

/**
 * @brief Copy-constructs a task.
 */
Task::Task(Task& other)
    : WorkItem(other),
      currentState(0),
      asignee(other.asignee),
      requiredRole(other.requiredRole),
      strategy(other.strategy),
      deadline(other.deadline),
      publisher(other.publisher) {
}

/**
 * @brief Creates an iterator over available tasks.
 */
WorkItemIterator* Task::createAvailableIterator() {
    return 0;
}

/**
 * @brief Gets the task's assignee.
 */
Participant* Task::getAssignee() {
    return asignee;
}

/**
 * @brief Sets the task's assignee.
 */
void Task::setAssignee(Participant* participant) {
    asignee = participant;
}

/**
 * @brief Gets the role required for the task.
 */
Role* Task::getRequiredRole() {
    return requiredRole;
}

/**
 * @brief Sets the task's assignment strategy.
 */
void Task::setAssignmentStrategy(AssignmentStrategy* strategy) {
    this->strategy = strategy;
}

/**
 * @brief Sets the automatic assignment strategy.
 */
void Task::autoAssignmentStrategy(AssignmentStrategy* strategy) {
    this->strategy = strategy;
}

/**
 * @brief Automatically assigns the task from candidates.
 */
void Task::autoAssign(std::vector<Participant*> candidates) {
    if (strategy != 0) {
        asignee = strategy->assign(candidates, requiredRole);
    }
}

/**
 * @brief Sets the task's deadline.
 */
void Task::setDeadlines(time_t* deadline) {
    if (deadline != 0) {
        this->deadline = *deadline;
    }
}

/**
 * @brief Gets the task's deadline.
 */
time_t Task::getDeadline() {
    return deadline;
}
