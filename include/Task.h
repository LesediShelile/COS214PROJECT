#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>
#include <ctime>
#include "TaskState.h"
#include "Participant.h"
#include "AssignmentStrategy.h"
#include "EventPublisher.h"
#include "Role.h"

/**
 * @brief Represents an individual task in the workflow.
 *
 * A Task maintains its current state, assignee, required role,
 * assignment strategy, deadline, and event publisher.
 */
class Task
{
private:
    TaskState* currentState;
    Participant* asignee;
    Role* requiredRole;
    AssignmentStrategy* strategy;
    time_t deadline;
    EventPublisher* publisher;

public:
    /**
     * @brief Constructs a Task.
     * @param itemID The task's identifier.
     * @param name The task's name.
     * @param role The role required to perform the task.
     * @param strategy The assignment strategy.
     */
    Task(std::string* itemID, std::string* name,
         Role* role, AssignmentStrategy* strategy);

    /** @brief Destroys the Task. */
    ~Task();

    /** @brief Makes the task available for assignment. */
    void makeAvailable();

    /**
     * @brief Assigns a participant to the task.
     * @param participant The participant to assign.
     */
    void assign(Participant* participant);

    /**
     * @brief Handles a change in the task's state.
     * @param newState The new state.
     */
    void handleChange(TaskState* newState);

    /** @brief Starts the task. */
    void start();

    /** @brief Marks the task as complete. */
    void complete();

    /**
     * @brief Rejects the task.
     * @param reason The reason for rejection.
     */
    void reject(std::string* reason);

    /** @brief Cancels the task. */
    void cancel();

    /**
     * @brief Checks whether the task has finished.
     * @return True if the task has finished.
     */
    bool isFinished();

    /**
     * @brief Checks whether the task is available.
     * @return True if the task is available.
     */
    bool isAvailable();

    /**
     * @brief Gets the task's current state name.
     * @return The state name.
     */
    std::string getStateName();

    /**
     * @brief Creates a copy of the task.
     * @return A pointer to the cloned WorkItem.
     */
    WorkItem* clone();

    /**
     * @brief Creates an iterator for the task.
     * @return A pointer to a WorkItemIterator.
     */
    WorkItemIterator* createIterator();

    /**
     * @brief Creates a memento of the task.
     * @return A pointer to a TaskMemento.
     */
    TaskMemento* createMemento();

    /**
     * @brief Restores the task from a memento.
     * @param memento The memento to restore.
     */
    void restoreMemento(TaskMemento* memento);

    /**
     * @brief Copy-constructs a task from another task.
     * @param other The task to copy.
     */
    Task(Task& other);

    /**
     * @brief Creates an iterator over available tasks.
     * @return A pointer to a WorkItemIterator.
     */
    WorkItemIterator* createAvailableIterator();

    /** @brief Gets the task's assignee. */
    Participant* getAssignee();

    /**
     * @brief Sets the task's assignee.
     * @param participant The participant to assign.
     */
    void setAssignee(Participant* participant);

    /** @brief Gets the role required for the task. */
    Role* getRequiredRole();

    /**
     * @brief Sets the task's assignment strategy.
     * @param strategy The assignment strategy.
     */
    void setAssignmentStrategy(AssignmentStrategy* strategy);

    /**
     * @brief Sets the automatic assignment strategy.
     * @param strategy The assignment strategy to use.
     */
    void autoAssignmentStrategy(AssignmentStrategy* strategy);

    /**
     * @brief Automatically assigns the task from a list of candidates.
     * @param candidates The available participants.
     */
    void autoAssign(std::vector<Participant*> candidates);

    /**
     * @brief Sets the task's deadline.
     * @param deadline The deadline to set.
     */
    void setDeadlines(time_t* deadline);

    /** @brief Gets the task's deadline. */
    time_t getDeadline();
};

#endif // TASK_H