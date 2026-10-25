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
 * Only Task (originator) can create a memento or read its contents,
 * so nothing else can see or alter a task's internal state. The memento owns its
 * TaskState, which must be a clone. 
   The assignee - non-owning pointer. WorkflowCommand (Caretaker) owns the memento.
 */
class TaskMemento {
public:
    ~TaskMemento();

    // Owns a TaskState, so no copying
    TaskMemento(const TaskMemento&) = delete;
    TaskMemento& operator=(const TaskMemento&) = delete;

private:
    friend class Task;

    //ownership of 'state'.
    TaskMemento(TaskState* state, Participant* assignee,
                const std::map<std::string, std::string>& data);

    TaskState* state;
    Participant* assignee;
    std::map<std::string, std::string> data;
};

#endif