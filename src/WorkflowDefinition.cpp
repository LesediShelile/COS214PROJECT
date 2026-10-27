#include "WorkflowDefinition.h"

WorkflowDefinition::WorkflowDefinition(string name, int version)
{
    this->name = name;
    this->version = version;
    this->approvalChain = nullptr;
    this->root = nullptr;
    this->frozen = false;
}

WorkItem* WorkflowDefinition::getRoot()
{
    return this->root;
}

void WorkflowDefinition::start()
{
    if(this->root == nullptr)
    {
        std::cout << "Error: Workflow definition is empty\n";
        return;
    }
    freeze();
    std::cout << "workflow definition: " << this->name <<  "v" << this->version << " started and frozen\n";
}

string WorkflowDefinition::getName()
{
    return this->name;
}

int WorkflowDefinition::getVersion()
{
    return this->version;
}

void WorkflowDefinition::setRoot(WorkItem* root)
{
    this->root = root;
}

ApprovalHandler* WorkflowDefinition::getApprovalChain()
{
    return this->approvalChain;
}

void WorkflowDefinition::setApprovalChain(ApprovalHandler* approvalChain)
{
    this->approvalChain = approvalChain;
}

void WorkflowDefinition::freeze()
{
    this->frozen = true;
}

bool WorkflowDefinition::isFrozen()
{
    return this->frozen;
}

WorkflowInstance* WorkflowDefinition::createInstance(string instanceID)
{
    return new WorkflowInstance(instanceID, this);
}

void WorkflowDefinition::requestApproval(ApprovalRequest* request)
{
    this->approvalChain->handle(request);
}