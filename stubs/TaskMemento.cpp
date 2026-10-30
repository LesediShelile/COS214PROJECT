// STUB build of the real include/TaskMemento.h (Kiara's header is used as is).
// The real src/TaskMemento.cpp includes ../include/TaskState.h, which does not
// exist until Cameron's branch is merged, so stub builds compile this file instead.
#include "TaskMemento.h"
#include "TaskState.h"

TaskMemento::TaskMemento(TaskState* state, Participant* assignee,
                         const std::map<std::string, std::string>& data)
    : state(state), assignee(assignee), data(data) {}

TaskMemento::~TaskMemento() { delete state; }
