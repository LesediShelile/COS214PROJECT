#ifndef WORKFLOW_DEFINITION_H
#define WORKFLOW_DEFINITION_H

#include <iostream>
#include <string>
using namespace std;

//Forward declarations
class WorkItem;
class ApprovalHandler;
class WorkInstance;

class WorkflowDefinition
{
    public:
        WorkflowDefinition(string name, int version);
        WorkItem* getRoot();
        void start();
        string getName();
        int getVersion();
        void setRoot(WorkItem* root);
        ApprovalHandler* setApprovalChain();
        void freeze();
        bool isFrozen();
        WorkInstance* createInstance(string instanceID);
        virtual ~WorkflowDefinition();
    private:
        WorkItem* root;
        string name;
        int version;
        ApprovalHandler* approvalChain;
        bool frozen;
  
};  

#endif //WORKFLOW_DEFINITION