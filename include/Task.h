#ifndef TASK_H
#define TASK_H
#include "TaskState.h"
#include "Participant.h"
#include "AssignmentStrategy.h"
#include "EventPublisher.h"
#include "Role.h"


class Task{

    private:
        TaskState* currentState;
        Participant* asignee;
        Role* requiredRole;
        AssignmentStrategy* strategy;
        time_t deadline;
        EventPublisher* publisher;

    public:
        Task(std::string itemID, std::string name, Role* role, AssignmentStrategy* strategy);
        ~Task();
        void makeAvailable();
        assign(Participant* participant);
        handleChange(TaskState* newState);
        void start();
        void complete();
        void reject(std::string reason);
        void cancel();
        bool isFinished();
        bool isAvailable();
        std::string getStateName();
        WorkItem* clone();
        WorkItemIterator* createIterator();
        TaskMemento* createMemento();
        void restoreMemento(TaskMemento* memento);
        Task(Task& other);
        WorkIteamIterator* createAvailableIterator();
        Participant* getAssignee();
        void setAssignee(Participant* participant);
        Role* getRequiredRole();
        void setAssignmentStrategy(AssignmentStrategy* strategy);
        void autoAssignmentStrategy(AssignmentStrategy* strategy);
        void autoAssign( std::vector<Participant* candidates);
        void setDeadlines(time_t deadline);
        time_t getDeadline();
        


    
};
#endif // WORKITEM_H