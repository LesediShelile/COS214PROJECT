/**
 * @file WorkflowInstance.cpp
 * @brief Implementation of the WorkflowInstance class managing execution lifecycle.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-27
 */

#include "../include/WorkflowInstance.h"

#include <ctime>
#include <memory>
#include <stdexcept>

#include "../include/EventPublisher.h"
#include "../include/WorkItem.h"
#include "../include/WorkItemIterator.h"
#include "../include/WorkflowDefinition.h"
#include "../include/WorkflowEvent.h"

namespace {
    /**
     * @brief Generates a formatted current timestamp string.
     * @return std::string Formatted date-time string (YYYY-MM-DD HH:MM:SS).
     */
    std::string currentTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm* local = std::localtime(&now);
        if (local == nullptr) {
            return "";
        }
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);
        return buffer;
    }
}

/**
 * @brief Constructs a new WorkflowInstance and clones the work item tree.
 * @param[in] instanceID The unique instance identifier.
 * @param[in] definition Pointer to the blueprint definition.
 * @throws std::invalid_argument If definition is null.
 * @throws std::logic_error If the definition has not been frozen.
 */
WorkflowInstance::WorkflowInstance(const std::string& instanceID, WorkflowDefinition* definition)
    : instanceID(instanceID),
      definition(definition),
      rootWorkItem(nullptr),
      status(WorkflowStatus::CREATED),
      publisher(nullptr) {
    if (definition == nullptr) {
        throw std::invalid_argument("WorkflowInstance: definition is null");
    }
    if (!definition->isFrozen()) {
        throw std::logic_error("WorkflowInstance: definition '" + definition->getName() +
                               "' must be frozen before instances are created");
    }

    //clean up by hand.
    try {
        publisher = new EventPublisher();
        rootWorkItem = definition->getRoot()->clone();   // Prototype
        connectPublisher();
    } catch (...) {
        delete rootWorkItem;
        delete publisher;
        throw;
    }
}

/**
 * @brief Destructor releasing the cloned tree and event publisher.
 */
WorkflowInstance::~WorkflowInstance() {
    delete rootWorkItem;
    delete publisher;
}

/**
 * @brief Retrieves the instance identifier.
 * @return std::string The unique instance ID.
 */
std::string WorkflowInstance::getInstanceID() const {
    return instanceID;
}

/**
 * @brief Retrieves the underlying workflow definition blueprint.
 * @return WorkflowDefinition* Pointer to the workflow definition.
 */
WorkflowDefinition* WorkflowInstance::getDefinition() const {
    return definition;
}

/**
 * @brief Retrieves the version number of the workflow definition.
 * @return int The version number.
 */
int WorkflowInstance::getDefinitionVersion() const {
    return definition->getVersion();
}

/**
 * @brief Retrieves the root of the instance's work item tree.
 * @return WorkItem* Pointer to the cloned root work item.
 */
WorkItem* WorkflowInstance::getRoot() const {
    return rootWorkItem;
}

/**
 * @brief Retrieves the current lifecycle execution status.
 * @return WorkflowStatus The current status enum value.
 */
WorkflowStatus WorkflowInstance::getStatus() const {
    return status;
}

/**
 * @brief Retrieves the recorded outcome message.
 * @return std::string The outcome description.
 */
std::string WorkflowInstance::getOutcome() const {
    return outcome;
}

/**
 * @brief Sets or updates the execution outcome message.
 * @param[in] newOutcome The new outcome description string.
 */
void WorkflowInstance::setOutcome(const std::string& newOutcome) {
    outcome = newOutcome;
}

/**
 * @brief Retrieves the instance's EventPublisher.
 * @return EventPublisher* Pointer to the publisher.
 */
EventPublisher* WorkflowInstance::getPublisher() const {
    return publisher;
}

/**
 * @brief Transitions the instance to RUNNING and exposes the initial tasks.
 * @throws std::logic_error If the instance status is not CREATED.
 */
void WorkflowInstance::start() {
    if (status != WorkflowStatus::CREATED) {
        throw std::logic_error("start: instance '" + instanceID + "' has already been started");
    }
    status = WorkflowStatus::RUNNING;
    try {
        rootWorkItem->makeAvailable();
    } catch (...) {
        status = WorkflowStatus::CREATED;
        throw;
    }
}

/**
 * @brief Advances active tasks or completes the workflow if all nodes are done.
 */
void WorkflowInstance::activateNext() {
    if (status != WorkflowStatus::RUNNING) {
        return;
    }
    if (rootWorkItem->isFinished()) {
        finish(WorkflowStatus::COMPLETED, "Completed");
        return;
    }
    //a stage releases its next eligible children and passes the
    // call down to nested stages, a task does nothing.
    rootWorkItem->activateChildren();
}

/**
 * @brief Cancels the instance and stops active work items.
 * @throws std::logic_error If the instance is already in a terminal state.
 */
void WorkflowInstance::cancel() {
    if (isFinished()) {
        throw std::logic_error("cancel: instance '" + instanceID + "' is already finished");
    }
    rootWorkItem->cancel();
    finish(WorkflowStatus::CANCELLED, "Cancelled");
}

/**
 * @brief Fails the instance with a specific explanation.
 * @param[in] reason Detailed explanation of the failure.
 * @throws std::logic_error If the instance is already in a terminal state.
 */
void WorkflowInstance::fail(const std::string& reason) {
    if (isFinished()) {
        throw std::logic_error("fail: instance '" + instanceID + "' is already finished");
    }
    rootWorkItem->cancel();
    if (!reason.empty()) {
        outcome = reason;
    }
    finish(WorkflowStatus::FAILED, "Failed");
}

/**
 * @brief Checks if the instance is in COMPLETED, CANCELLED, or FAILED status.
 * @return true If execution has concluded, false otherwise.
 */
bool WorkflowInstance::isFinished() const {
    return status == WorkflowStatus::COMPLETED ||
           status == WorkflowStatus::CANCELLED ||
           status == WorkflowStatus::FAILED;
}

/**
 * @brief Gathers all items currently available for execution using an iterator.
 * @return std::vector<WorkItem*> List of pointers to available WorkItems.
 */
std::vector<WorkItem*> WorkflowInstance::getAvailableItems() const {
    std::vector<WorkItem*> available;
    // Iterator: walk the tree without knowing how stages store their children.
    std::unique_ptr<WorkItemIterator> it(rootWorkItem->createAvailableIterator());
    for (it->first(); !it->isDone(); it->next()) {
        available.push_back(it->current());
    }
    return available;
}

/**
 * @brief Finalizes workflow status and dispatches the WORKFLOW_FINISHED event to observers.
 * @param[in] finalStatus The concluding status.
 * @param[in] defaultOutcome Fallback description if outcome is currently blank.
 */
void WorkflowInstance::finish(WorkflowStatus finalStatus, const std::string& defaultOutcome) {
    status = finalStatus;
    if (outcome.empty()) {
        outcome = defaultOutcome;
    }
    // Observer: tell every listener the workflow has ended.
    WorkflowEvent event(EventType::WORKFLOW_FINISHED, instanceID, currentTimestamp(), outcome);
    publisher->publish(&event);
}

/**
 * @brief Propagates the EventPublisher to all nodes in the cloned tree.
 */
void WorkflowInstance::connectPublisher() {
    // Give every item in the cloned tree this instance's publisher.

    std::unique_ptr<WorkItemIterator> it(rootWorkItem->createIterator());
    for (it->first(); !it->isDone(); it->next()) {
        it->current()->setPublisher(publisher);
    }
}