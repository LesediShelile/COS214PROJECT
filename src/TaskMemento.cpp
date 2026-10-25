#include "../include/TaskMemento.h"

#include "../include/TaskState.h"

TaskMemento::TaskMemento(TaskState* state, Participant* assignee,
                         const std::map<std::string, std::string>& data)
    : state(state), assignee(assignee), data(data) {}

TaskMemento::~TaskMemento() {
    delete state;
}