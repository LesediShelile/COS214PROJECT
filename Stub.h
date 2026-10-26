#ifndef STUB_H
#define STUB_H

#include <iostream>
#include <vector>

#include "WorkflowDefinition.h"

class WorkItem
{
    public:
       virtual std::vector<WorkItem*> getChildren();
       virtual bool isAvailable();
       void addChild(WorkItem* item);
};

class Role
{
    public:
        Role(string name, int authorityLevel);
};

class ExternalConnector
{

};

class EventPublisher
{

};

class WorkflowInstance
{
    public:
        WorkflowInstance(string instanceID, WorkflowDefinition* definition);
};

class ApprovalRequest
{

};

class ApprovalHandler
{
    public:
        void handle(ApprovalRequest*);
};

class Stage : public WorkItem
{
    public:
        Stage(string name,string itemID);
};

class SequentialStage : public Stage
{
    public:
     SequentialStage(string name, string itemID);
};

class ParallelStage : public Stage
{
    public:
     ParallelStage(string name, string itemID);
};

class Task : public WorkItem
{
    public:
        Task(string, string, Role*, AssignmentStrategy*);
};

class AssignmentStrategy
{

};

class ExternalTask: public Task
{
    public:
        ExternalTask(string itemID, string name, Role* role, AssignmentStrategy* strategy, ExternalConnector* connector, string request);
};

class AuditedTask : public WorkItem
{
    public:
        AuditedTask(WorkItem* task, EventPublisher* publisher);
};

class ValidatedTask : public WorkItem
{
    public:
        ValidatedTask(WorkItem* task, vector<string> fields);
};

class PriorityTask : public WorkItem
{
    public:
        PriorityTask(WorkItem* task, int level);
};

class AssignmentStrategy
{

};

class RoleBasedAssignment : public AssignmentStrategy
{

};
#endif