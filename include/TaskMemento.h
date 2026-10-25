#ifndef TASKMEMENTO_H
#define TASKMEMENTO_H

#include <map>
#include <string>

class TaskState;
class Participant;
class Task;

/**
 * Memento: a private snapshot of a Task taken before a command changes it.
 *
 * Only Task (the Originator) can create a memento or read its contents,
 * so nothing else can see or alter a task's internals. The memento owns its
 * TaskState, which must be a clone (see Task::createMemento). The assignee is a
 * non-owning pointer. A WorkflowCommand (the Caretaker) owns the memento.
 */
class TaskMemento {
public:
    ~TaskMemento();

    // Owns a TaskState, so copying is disabled.
    TaskMemento(const TaskMemento&) = delete;
    TaskMemento& operator=(const TaskMemento&) = delete;

private:
    friend class Task;

    // Takes ownership of 'state'.
    TaskMemento(TaskState* state, Participant* assignee,
                const std::map<std::string, std::string>& data);

    TaskState* state;
    Participant* assignee;
    std::map<std::string, std::string> data;
};

#endif