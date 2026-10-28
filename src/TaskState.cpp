#include "TaskState.h"
#include "Task.h"
#include <iostream>

TaskState::~TaskState() {}

// Default behaviour for every action: not allowed in this state.
// Each concrete state overrides only the actions it allows.

void TaskState::assign(Task* task, Participant*) {
    std::cout << "Cannot assign task '" << task->getName()
              << "' while it is " << getName() << std::endl;
    throw std::logic_error("Cannot assign task '" + task->getName() + "' while it is " + getName());          
}

void TaskState::start(Task* task) {
    std::cout << "Cannot start task '" << task->getName()
              << "' while it is " << getName() << std::endl;
              throw std::logic_error("Cannot start task '" + task->getName() + "' while it is " + getName());
}

void TaskState::complete(Task* task) {
    std::cout << "Cannot complete task '" << task->getName()
              << "' while it is " << getName() << std::endl;
              throw std::logic_error("Cannot complete task '" + task->getName() + "' while it is " + getName());
}

void TaskState::reject(Task* task, std::string) {
    std::cout << "Cannot reject task '" << task->getName()
              << "' while it is " << getName() << std::endl;
              throw std::logic_error("Cannot reject task '" + task->getName() + "' while it is " + getName());
}

void TaskState::cancel(Task* task) {
    std::cout << "Cannot cancel task '" << task->getName()
              << "' while it is " << getName() << std::endl;
              throw std::logic_error("Cannot cancel task '" + task->getName() + "' while it is " + getName());
}

void TaskState::makeAvailable(Task* task) {
    std::cout << "Cannot make task '" << task->getName()
              << "' available while it is " << getName() << std::endl;
              throw std::logic_error("Cannot make task '" + task->getName() + "' available while it is " + getName());
}


