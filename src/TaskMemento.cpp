/**
 * @file TaskMemento.cpp
 * @brief Implementation of the TaskMemento class representing an encapsulated Task snapshot.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#include "../include/TaskMemento.h"

#include "../include/TaskState.h"

/**
 * @brief Constructs a TaskMemento capturing the internal state of a Task.
 * @param[in] state Pointer to the cloned TaskState representing current lifecycle state.
 * @param[in] assignee Non-owning pointer to the currently assigned Participant.
 * @param[in] data Key-value metadata attributes associated with the task.
 */
TaskMemento::TaskMemento(TaskState* state, Participant* assignee,
                         const std::map<std::string, std::string>& data)
    : state(state), assignee(assignee), data(data) {}

/**
 * @brief Destructor deallocating the captured TaskState object.
 */
TaskMemento::~TaskMemento() {
    delete state;
}