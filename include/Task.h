#ifndef TASK_H
#define TASK_H

#include <ctime>
#include <string>
#include <vector>
#include "WorkItem.h"

class TaskState;
class Participant;
class Role;
class AssignmentStrategy;
class EventPublisher;

/**
 * @brief Leaf of the Composite, Context of State and Strategy, and
 *        ConcreteComponent of Decorator.
 *
 * A Task never decides what an operation does: it hands the call to its
 * current TaskState, which either performs it (and calls handleChange() with
 * the next state) or throws std::logic_error for an invalid operation.
 */
class Task : public WorkItem {
public:
    Task(const std::string& itemID, const std::string& name,
         Role* requiredRole, AssignmentStrategy* strategy);
    /** Copy constructor: clones the current state object. Participants, the
     *  role, the strategy and the publisher are shared, not owned. */
    Task(const Task& other);
    virtual ~Task();

    /** @name Lifecycle (delegated to the current TaskState) */
    ///@{
    virtual void makeAvailable();
    virtual void assign(Participant* participant);
    virtual void start();
    virtual void complete();
    virtual void reject(const std::string& reason);
    virtual void cancel();
    ///@}

    virtual bool isFinished() const;
    virtual bool isAvailable() const;
    virtual std::string getStateName() const;

    /**
     * @brief Called by a TaskState to move this task to its next state.
     * Takes ownership of @p newState and deletes the previous state, so a
     * state must make this its LAST statement and not touch itself afterwards.
     * @todo Lesedi: publish a WorkflowEvent through publisher_ here.
     */
    void handleChange(TaskState* newState);

    /** @brief Prototype: deep copy including the state object. */
    virtual WorkItem* clone() const;

    virtual TaskMemento* createMemento();
    virtual void restoreMemento(TaskMemento* memento);

    Participant* getAssignee() const;
    void setAssignee(Participant* participant);
    Role* getRequiredRole() const;

    void setAssignmentStrategy(AssignmentStrategy* strategy);
    /** @brief Ask the strategy for a participant and assign the task to them.
     *  @throws std::logic_error if no strategy is set,
     *  @throws std::runtime_error if the strategy finds nobody. */
    void autoAssign(const std::vector<Participant*>& candidates);

    void setDeadline(std::time_t deadline);
    std::time_t getDeadline() const;

    virtual void setPublisher(EventPublisher* publisher);
    EventPublisher* getPublisher() const;

private:
    TaskState* currentState_;
    Participant* assignee_;
    Role* requiredRole_;
    AssignmentStrategy* strategy_;
    std::time_t deadline_;
    EventPublisher* publisher_;

    Task& operator=(const Task&);
};

#endif
