#ifndef DEFINITION_BUILDER_H
#define DEFINITION_BUILDER_H

#include <iostream>
#include <stack>
#include <string>
using namespace std;

//classes included
#include "WorkflowBuilder.h"
#include "WorkflowDefinition.h"

//Forward declarations
class WorkflowDefinition;
class Stage;
class WorkItem;
class Role;
class AssignmentStrategy;
class ExternalConnector;
class EventPublisher;
class ApprovalHandler;

/**
 * @brief Builder Pattern: Concrete Builder
 * This takes the interface defined by WorkflowBuilder and creates a 
 * workflow definition tailored to a specific scenario
 */



class DefinitionBuilder : public WorkflowBuilder
{
    public:

        /**
         * @brief constructor that sets members appropriately
         */
        DefinitionBuilder();

        /**
         * @brief Returns the current workflow definition
         * will send a warning if there is uncommited changes
         * @return The created definition
         */
        WorkflowDefinition* getResult() override;

        /**
         * @brief Clears the current worflow definition so a new one
         * may be created by popping the stage stack
         * @param name the name of the new workflow definition
         * @param version the verstion of the new workflow definition
         */
        void reset(string name, int version) override;

        /**
         * @brief Creates a new Sequential stage that gets pushed to the 
         * stage stack
         * @param name The name of the new Stage
         */
        void beginSequentialStage(string name) override;

        /**
         * @brief Creates a new Parallel stage that gets pushed to the stage stack
         * @param name The name of the new Stage
         */
        void beginParallelStage(string name) override;

         /**
         * @brief Creates a new Task and increments the item Ids
         * Sets the new task as the pending task
         * @param name The name of the new task
         * @param role Sets the person in charge of the new created task
         * @param strategy Sets how the new task will be completed
         */
        void addTask(string name, Role* role, AssignmentStrategy* strategy) override;

        /**
         * @brief This adds a new external task via the adapter and sets it as 
         * the pending task
         * @param name The name of the new Task
         * @param role The person in charge of the external task
         * @param strategy Sets how the task will be completed
         * @param connector Sets the connector for the Adaptee classes to become External Tasks
         * @param request Sets the request that is attached to the External Task
         */
        void addExternalTask(string name, Role* role, AssignmentStrategy* strategy, ExternalConnector* connector, string request) override;

        /**
         * @brief Uses the decorator to wrap a the pending task with an Audited label
         * @param publisher Sets the publisher to audit the task via the Observer pattern
         */
        void withAudit(EventPublisher* publisher) override;

        /**
         * @brief Uses the decorator to wrap the pending task with a validated label
         * @param fields sets the required fields for creating a new Validated task
         */
        void withValidation(vector<string> fields) override;

        /**
         * @brief Uses the decorator to wrap the pending task with a Priority label
         * @param level Sets the priority level for the new Priority Task
         */
        void withPriority(int level) override;

        /**
         * @brief Pops the stage currently at the top of the stack
         */
        void endStage() override;

        /**
         * @brief Adds the pending task as a child of the stage that
         * is currently at the top of the stack
         */
        void commitPendingTask();

        /**
         * @brief deallocates any allocated memory that is owned by this class
         */
        virtual ~DefinitionBuilder();

    private:
        /**
         * @brief the current worflow definition
         */
        WorkflowDefinition* definition;

        /**
         * @brief The stack that stores the creates stages of the definition
         */
        stack<Stage*> stageStack;

        /**
         * @brief The current Task that needs to be allocated into the stack stages
         */
        WorkItem* pendingTask;

        /**
         * @brief the unique ID of work items
         */
        int nextItemID;
};

#endif //DEFINITIONBUILDER