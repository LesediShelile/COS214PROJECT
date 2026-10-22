    #ifndef WORKFLOW_BUILDER_H
    #define WORKFLOW_BUILDER_H

    #include <iostream>
    #include <vector>
    #include <string>
    using namespace std;

    //Forward declarations
    class WorkflowDefinition;
    class Role;
    class AssignmentStrategy;
    class ExternalConnector;
    class EventPublisher;
    class ApprovalHandler;

    class WorkflowBuilder
    {
        public:
            WorkflowBuilder();
            virtual WorkflowDefinition* getResult() = 0;
            virtual void reset(string, int) = 0;
            virtual void beginSequentialStage(string) = 0;
            virtual void beginParallelStage(string) = 0;
            virtual void addTask(string, Role*, AssignmentStrategy*) = 0;
            virtual void addExternalTask(string, ExternalConnector*, string) = 0;
            virtual void withAudit(EventPublisher*) = 0;
            virtual void withValidation(vector<string>) = 0;
            virtual void withPriority(int) = 0;
            virtual void setApprovalChain(ApprovalHandler*) = 0;
            virtual void endStage();
            virtual ~WorkflowBuilder();
    };

    #endif //WORKFLOWBUILDER