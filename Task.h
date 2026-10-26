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
class TaskMemento;
class WorkItemIterator;

class Task : public WorkItem {
public:
    Task(std::string itemID, std::string name, Role* role, AssignmentStrategy* strategy);
    Task(const Task& other);
    ~Task();

    // State pattern: these forward to currentState
    void makeAvailable() override;
    void assign(Participant* participant) override;
    void start() override;
    void complete() override;
    void reject(std::string reason) override;
    void cancel() override;

    void handleChange(TaskState* newState);
    std::string getStateName() override;
    bool isFinished() override;
    bool isAvailable() override;

    // Prototype
    WorkItem* clone() override;

    // Iterator
    WorkItemIterator* createIterator() override;
    WorkItemIterator* createAvailableIterator() override;

    // Memento
    TaskMemento* createMemento() override;
    void restoreMemento(TaskMemento* memento) override;

    Participant* getAssignee();
    void setAssignee(Participant* participant);
    Role* getRequiredRole();

    // Strategy
    void setAssignmentStrategy(AssignmentStrategy* strategy);
    void autoAssign(std::vector<Participant*> candidates);

    void setDeadline(time_t deadline);
    time_t getDeadline();

private:
    TaskState* currentState;
    Participant* asignee;
    Role* requiredRole;
    AssignmentStrategy* strategy;
    time_t deadline;
    EventPublisher* publisher;
};

#endif