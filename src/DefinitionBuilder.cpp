#include "DefinitionBuilder.h"
#include <vector>
#include "WorkItem.h"
#include "Stage.h"
#include "SequentialStage.h"
#include "ParallelStage.h"
#include "Task.h"
#include "ExternalTask.h"
#include "AuditedTask.h"
#include "ValidatedTask.h"
#include "PriorityTask.h"
#include "Role.h"
#include "AssignmentStrategy.h"
#include "ExternalConnector.h"
#include "EventPublisher.h"
#include "ApprovalHandler.h"

DefinitionBuilder::DefinitionBuilder()
{
    this->definition = nullptr;
    this->pendingTask = nullptr;
    this->nextItemID = 0;
}

WorkflowDefinition* DefinitionBuilder::getResult()
{
    if(!stageStack.empty())
    {
        cout << "Warning: returning workflow definition with unclosed changes on the stack.\n";
    }
    WorkflowDefinition* result = this->definition;

    return result;
}

void DefinitionBuilder::reset(string name, int version)
{
    delete this->definition;
    this->definition = new WorkflowDefinition(name, version);

    while(!stageStack.empty())
    {
        this->stageStack.pop();
    }

    this->pendingTask = nullptr;
    this->nextItemID = 0;
}

void DefinitionBuilder::beginSequentialStage(string name)
{
    Stage* newStage = new SequentialStage(name, to_string(this->nextItemID));
    this->nextItemID++;

    if(this->stageStack.empty())
    {
        if(this->definition != nullptr)
        {
            this->definition->setRoot(newStage);
        }

    }
    else
    {
        this->stageStack.top()->addChild(newStage);
    }
    this->stageStack.push(newStage);

}

void DefinitionBuilder::beginParallelStage(string name)
{
    Stage* newStage = new ParallelStage(name, to_string(this->nextItemID));
    this->nextItemID++;

    if(this->stageStack.empty())
    {
        if(this->definition != nullptr)
        {
            this->definition->setRoot(newStage);
        }

    }
    else
    {
        this->stageStack.top()->addChild(newStage);
    }
    this->stageStack.push(newStage);

}

void DefinitionBuilder::addTask(string name, Role* role, AssignmentStrategy* strategy)
{
    WorkItem* newTask = new Task(to_string(this->nextItemID), name, role, strategy);
    this->nextItemID++;

    this->pendingTask = newTask;
}

void DefinitionBuilder::addExternalTask(string name, Role* role, AssignmentStrategy* strategy, ExternalConnector* connector, string request)
{
    WorkItem* newExternalTask = new ExternalTask(to_string(this->nextItemID), name, role, strategy, connector, request);
    this->nextItemID++;

    this->pendingTask = newExternalTask;
}

void DefinitionBuilder::withAudit(EventPublisher* publisher)
{
    if(this->pendingTask == nullptr){return;}
    
    WorkItem* auditTask =  new AuditedTask(this->pendingTask, publisher);
    

    //NB. Swap out the old task if needed
    //check how composite stores tasks

    this->pendingTask = auditTask;
}

void DefinitionBuilder::withValidation(vector<string> fields)
{
    if(this->pendingTask == nullptr){return;}
    
    WorkItem* VTask =  new ValidatedTask(this->pendingTask, fields);
    

    //NB. Swap out the old task if needed
    //check how composite stores tasks

    this->pendingTask = VTask;
}

void DefinitionBuilder::withPriority(int level)
{
    if(this->pendingTask == nullptr){return;}
    
    WorkItem* PTask =  new PriorityTask(this->pendingTask, level);
    

    //NB. Swap out the old task if needed
    //check how composite stores tasks

    this->pendingTask = PTask;
}

void DefinitionBuilder::endStage()
{
    if(!this->stageStack.empty())
    {
        this->stageStack.pop();
    }
}

void DefinitionBuilder::commitPendingTask()
{
    if(!this->stageStack.empty())
    {
        this->stageStack.top()->addChild(pendingTask);
    }
    this->pendingTask = nullptr;
}

DefinitionBuilder::~DefinitionBuilder()
{
    delete this->definition;

}