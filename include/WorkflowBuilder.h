    
    
    #ifndef WORKFLOW_BUILDER_H
    #define WORKFLOW_BUILDER_H

    #include <iostream>
    #include <vector>
    #include <string>

    //Forward declarations
    class WorkflowDefinition;
    class Role;
    class AssignmentStrategy;
    class ExternalConnector;
    class EventPublisher;
    class ApprovalHandler;

    /**
     * @brief Builder: Provides an interface for the concrete builders
     * 
     * Consists of pure virtual functions that the concrete builders 
     * implement according to their functionality.
     * Allows for seamless addition of different builders for
     * different workflow scenarios
     */

    class WorkflowBuilder
    {
        public:

            /**
             * @brief Default constructor as no members need to be assigned
             */
            WorkflowBuilder() = default;

            /**
             * @brief returns the current workflow definition that has been created
             */
            virtual WorkflowDefinition* getResult() = 0;

            /**
             * @brief resets the current workflow definition so a new one
             * may be created
             * @param name The name of the new workflow definition
             * @param version assigns the new workflow a version
             * @return The created definition
             */
            virtual void reset(std::string name, int version) = 0;

            /**
             * @brief Creates a new sequential stage and pushes it onto the the workflow
             * definition's stack
             * @param name The name of the new Sequential stage object that is
             * created
             */
            virtual void beginSequentialStage(std::string name) = 0;

            /**
             * @brief Creates a new Parallel stage and pushes it onto the workflow
             * definition's stack
             * @param name The name of the new Parallel stage that is created
             */
            virtual void beginParallelStage(std::string) = 0;

            /**
             * @brief Creates a new Task
             * @param name The name of the new task
             * @param role Sets the person in charge of the new created task
             * @param strategy Sets how the new task will be completed
             */
            virtual void addTask(std::string name, Role* role, AssignmentStrategy* strategy) = 0;

            /**
             * @brief This adds a new external task via the adapter
             * @param name The name of the new Task
             * @param role The person in charge of the external task
             * @param strategy Sets how the task will be completed
             * @param connector Sets the connector for the Adaptee classes to become External Tasks
             * @param request Sets the request that is attached to the External Task
             */
            virtual void addExternalTask(std::string name, Role* role, AssignmentStrategy* strategy, ExternalConnector* connector, string request) = 0;

            /**
             * @brief Uses the decorator to wrap a the pending task with an Audited label
             * @param publisher Sets the publisher to audit the task via the Observer pattern
             */
            virtual void withAudit(EventPublisher* publisher) = 0;

            /**
             * @brief Uses the decorator to wrap the pending task with a validated label
             * @param fields sets the required fields for creating a new Validated task
             */
            virtual void withValidation(std::vector<std::string> fields) = 0;

            /**
             * @brief Uses the decorator to wrap the pending task with a Priority label
             * @param level Sets the priority level for the new Priority Task
             */
            virtual void withPriority(int level) = 0;

            /**
             * @brief Delegates to the derived classes how they will end a stage
             */

            virtual void endStage() = 0;
            /**
             * @brief Delegates to the derived classes how to commit pending tasks
             */
            virtual void commitPendingTask() = 0;

            /**
             * @brief Virtual Default destructor
             */
            virtual ~WorkflowBuilder() = default;
    };

    #endif //WORKFLOWBUILDER