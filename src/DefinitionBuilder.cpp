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
    if(this->definition == nullptr)
    {
        throw std::logic_error("getResult: There is no definition to return");
    }
    
    if(!stageStack.empty())
    {
        throw std::logic_error("getResult: Please end any open stages before returning definition");
    }
    if(this->pendingTask != nullptr)
    {
        throw std::logic_error("getResult: Please commit the pending task before returning definiton");
    }
    if(this->definition->getRoot() == nullptr)
    {
        throw std::logic_error("getResult: Cannot return empty definition");
    }
    WorkflowDefinition* result = this->definition;
    this->definition = nullptr; 
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

    delete this->pendingTask;
    this->pendingTask = nullptr;
    this->nextItemID = 0;
}

void DefinitionBuilder::beginSequentialStage(string name)
{
    if(this->definition == nullptr)
    {
        throw std::logic_error("beginSequentialStage: call reset before building a workflow");
    }
    
    if (stageStack.empty() && definition->getRoot() != nullptr) 
    {
         throw std::logic_error( "beginSequentialStage: workflow already has a root stage");
    }   
    if(this->pendingTask != nullptr)
    {
        throw std::logic_error("beginSequentialStage: Please commit the pending task before opening a new Stage");
    } 
    Stage* newStage = new SequentialStage(to_string(this->nextItemID), name);
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
        this->stageStack.top()->add(newStage);
    }
    this->stageStack.push(newStage);

}

void DefinitionBuilder::beginParallelStage(string name)
{
    if(this->definition == nullptr)
    {
        throw std::logic_error("beginParallelStage: call reset before building a workflow");
    }

    if (stageStack.empty() && definition->getRoot() != nullptr) 
    {
         throw std::logic_error( "beginParallelStage: workflow already has a root stage");
    } 
    if(this->pendingTask != nullptr)
    {
        throw std::logic_error("beginParallelStage: Please commit the pending task before opening a new Stage");
    } 
    
    Stage* newStage = new ParallelStage(to_string(this->nextItemID), name);
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
        this->stageStack.top()->add(newStage);
    }
    this->stageStack.push(newStage);

}

void DefinitionBuilder::addTask(string name, Role* role, AssignmentStrategy* strategy)
{
    if(this->definition == nullptr)
    {
        throw std::logic_error("addTask: call reset before building a workflow");
    }
    
    if(this->pendingTask != nullptr)
    {
        throw std::logic_error("addTask: commit the current pending task before adding a new task");
    }
    
    if(this->stageStack.empty())
    {
        throw std::logic_error("addTask: the stage stack is empty, task cannot be added");
    }
    WorkItem* newTask = new Task(to_string(this->nextItemID), name, role, strategy);
    this->nextItemID++;

    this->pendingTask = newTask;
}

void DefinitionBuilder::addExternalTask(string name, Role* role, AssignmentStrategy* strategy, ExternalConnector* connector, string request)
{
   
    if(this->definition == nullptr)
    {
        throw std::logic_error("addExternalTask: call reset before building a workflow");
    }
    
    if(this->pendingTask != nullptr)
    {
        throw std::logic_error("addExternalTask: commit the current pending task before adding a new task");
    }
    if(this->stageStack.empty())
    {
        throw std::logic_error("addExternalTask: the stage stack is empty, task cannot be added");
    }
    WorkItem* newExternalTask = new ExternalTask(to_string(this->nextItemID), name, role, strategy, connector, request);
    this->nextItemID++;

    this->pendingTask = newExternalTask;
}

void DefinitionBuilder::withAudit(EventPublisher* publisher)
{
    if(this->pendingTask == nullptr)
    {
        throw std::logic_error("withAudit: there is no pending task to decorate");
    }
    
    WorkItem* auditTask =  new AuditedTask(this->pendingTask, publisher);
    
    this->pendingTask = auditTask;
}

void DefinitionBuilder::withValidation(vector<string> fields)
{
    if(this->pendingTask == nullptr)
    {
        throw std::logic_error("withValidation: there is no pending task to decorate");
    }
    
    WorkItem* VTask =  new ValidatedTask(this->pendingTask, fields);
    

    this->pendingTask = VTask;
}

void DefinitionBuilder::withPriority(int level)
{
    if(this->pendingTask == nullptr)
    {
        throw std::logic_error("withPriority: there is no pending task to decorate");
    }
    
    WorkItem* PTask =  new PriorityTask(this->pendingTask, level);
    

    this->pendingTask = PTask;
}

void DefinitionBuilder::endStage()
{
    if(this->pendingTask != nullptr)
    {
        throw std::logic_error("endStage: Commit any pending tasks before ending a stage");
    }
    if(!this->stageStack.empty())
    {
        this->stageStack.pop();
    }
    else
    {
        throw std::logic_error("endStage: The stage stack is currently empty");
    }
}

void DefinitionBuilder::commitPendingTask()
{
    if(this->pendingTask == nullptr)
    {
        throw std::logic_error("commitPendingTask: There is no current pending task to commit");
    }
    
    if(!this->stageStack.empty())
    {
        this->stageStack.top()->add(pendingTask);
        this->pendingTask = nullptr;
    }
    else
    {
        throw std::logic_error("commitPendingTask: The stage stack is empty");
    }

}

DefinitionBuilder::~DefinitionBuilder()
{
    delete this->definition;
    delete this->pendingTask;
}