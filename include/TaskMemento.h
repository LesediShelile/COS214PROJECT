/**
 * @file TaskMemento.h
 * @brief Header file declaring the TaskMemento class for saving snapshots of Task state.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef TASKMEMENTO_H
#define TASKMEMENTO_H

#include <map>
#include <string>

class TaskState;
class Participant;
class Task;

/**
 * @class TaskMemento
 * @brief Memento capturing a private snapshot of a Task prior to command execution.
 * 
 * Part of the Memento design pattern. Preserves the internal state of a Task
 * (originator), including its lifecycle state, current assignee, and data attributes.
 * Construction and inspection are restricted to Task via friendship, ensuring
 * encapsulation is preserved. The memento assumes ownership of the contained
 * TaskState, which must be a cloned instance, while storing a non-owning pointer
 * to the assignee. Instances of TaskMemento are managed and owned by WorkflowCommand
 * (the caretaker).
 * 
 * @see Task
 * @see TaskState
 * @see Participant
 * @see WorkflowCommand
 */
class TaskMemento {
public:
    /**
     * @brief Destructor deallocating the owned TaskState instance.
     */
    ~TaskMemento();

    /**
     * @brief Deleted copy constructor to maintain single ownership of the internal TaskState.
     */
    TaskMemento(const TaskMemento&) = delete;

    /**
     * @brief Deleted copy assignment operator to maintain single ownership of the internal TaskState.
     */
    TaskMemento& operator=(const TaskMemento&) = delete;

private:
    friend class Task;

    /**
     * @brief Private constructor called by Task to capture a snapshot.
     * @details Takes ownership of the provided TaskState pointer.
     * @param[in] state Cloned pointer representing the current TaskState.
     * @param[in] assignee Non-owning pointer to the currently assigned Participant.
     * @param[in] data Key-value map holding task custom metadata.
     */
    TaskMemento(TaskState* state, Participant* assignee,
                const std::map<std::string, std::string>& data);

    TaskState* state;                         ///< Owned pointer to the cloned task lifecycle state.
    Participant* assignee;                    ///< Non-owning pointer to the assigned participant.
    std::map<std::string, std::string> data;  ///< Snapshot of the task's metadata key-value pairs.
};

#endif