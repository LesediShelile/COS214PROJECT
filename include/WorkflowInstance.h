#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <vector>

class WorkflowDefinition;
class WorkItem;
class EventPublisher;


//Where a workflow instance is in its life.

enum class WorkflowStatus {
    CREATED,
    RUNNING,
    COMPLETED,
    CANCELLED,
    FAILED
};

/*
 * One running copy of a workflow definition.
 *
 * It owns its own clone of the definition's work-item tree and its own
 * EventPublisher, so changing one instance never affects another. 
 */

class WorkflowInstance {
public:
   

    WorkflowInstance(const std::string& instanceID, WorkflowDefinition* definition);
    ~WorkflowInstance();

    // Owns a tree and a publisher, so copying is disabled.
    WorkflowInstance(const WorkflowInstance&) = delete;
    WorkflowInstance& operator=(const WorkflowInstance&) = delete;

    std::string getInstanceID() const;
    WorkflowDefinition* getDefinition() const;
    int getDefinitionVersion() const;
    WorkItem* getRoot() const;
    WorkflowStatus getStatus() const;
    std::string getOutcome() const;
    void setOutcome(const std::string& outcome);
    EventPublisher* getPublisher() const;

    // Moves CREATED -> RUNNING
    void start();

    // Called after an item completes - EngineRouter. Finishes the workflow
    // if all work is done, otherwise releases the next eligible work.
    // Does nothing unless the instance is RUNNING.
    void activateNext();

    // Stops all outstanding work. Throws logic_error if already finished.
    void cancel();

    // Stops all outstanding work and records why.
    void fail(const std::string& reason);

    bool isFinished() const;

    // Items that can currently be assigned, found with an AvailableItemsIterator.
    std::vector<WorkItem*> getAvailableItems() const;

private:
    void finish(WorkflowStatus finalStatus, const std::string& defaultOutcome);
    void connectPublisher();

    std::string instanceID;
    WorkflowDefinition* definition;
    WorkItem* rootWorkItem;
    WorkflowStatus status;
    std::string outcome;
    EventPublisher* publisher;
};

#endif