#ifndef DEFINITION_BUILDER_H
#define DEFINITION_BUILDER_H

#include <iostream>
#include <stack>
#include <string>
using namespace std;

//classes included
#include "WorkflowBuilder.h"
#include "WorkflowDefinition.h"
#include "Stub.h"

//Forward declarations
class WorkflowDefinition;
class Stage;
class WorkItem;
class Role;
class AssignmentStrategy;
class ExternalConnector;
class EventPublisher;
class ApprovalHandler;


class DefinitionBuilder : public WorkflowBuilder
{
    public:
        DefinitionBuilder();
        WorkflowDefinition* getResult() override;
        void reset(string name, int version) override;
        void beginSequentialStage(string name) override;
        void beginParallelStage(string name) override;
        void addTask(string name, Role* role, AssignmentStrategy* strategy) override;
        void addExternalTask(string name, Role* role, AssignmentStrategy* strategy, ExternalConnector* connector, string request) override;
        void withAudit(EventPublisher* publisher) override;
        void withValidation(vector<string> fields) override;
        void withPriority(int level) override;
        void endStage() override;
        void commitPendingTask();
        virtual ~DefinitionBuilder();

    private:
        WorkflowDefinition* definition;
        stack<Stage*> stageStack;
        WorkItem* pendingTask;
        int nextItemID;
};

#endif //DEFINITIONBUILDER