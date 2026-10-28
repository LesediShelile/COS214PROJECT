/**
 * @file WorkflowInstance.h
 * @brief Declaration of the WorkflowInstance class representing an active execution instance.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-27
 */

#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <vector>

class WorkflowDefinition;
class WorkItem;
class EventPublisher;

/**
 * @enum WorkflowStatus
 * @brief Represents the lifecycle execution states of a workflow instance.
 */
enum class WorkflowStatus {
    CREATED,   ///< The instance has been constructed but not yet started.
    RUNNING,   ///< The instance is actively executing work items.
    COMPLETED, ///< All items in the workflow tree completed successfully.
    CANCELLED, ///< Execution was aborted externally.
    FAILED     ///< Execution terminated due to a failure or error condition.
};

/**
 * @class WorkflowInstance
 * @brief Represents a running copy of a workflow definition.
 * 
 * Owns an isolated clone of the definition's work item tree (via Prototype)
 * and its own EventPublisher (via Observer), ensuring that state updates within
 * one instance do not interfere with other active workflows.
 * 
 * @see WorkflowDefinition
 * @see WorkItem
 * @see EventPublisher
 */
class WorkflowInstance {
public:
    /**
     * @brief Constructs an instance by cloning the definition's prototype tree.
     * @param[in] instanceID Unique identifier for this instance.
     * @param[in] definition Pointer to the parent WorkflowDefinition blueprint.
     * @throws std::invalid_argument If definition is null.
     * @throws std::logic_error If definition is not frozen.
     */
    WorkflowInstance(const std::string& instanceID, WorkflowDefinition* definition);

    /**
     * @brief Destructor deallocating the cloned work item tree and EventPublisher.
     */
    ~WorkflowInstance();

    /**
     * @brief Deleted copy constructor to prevent shared ownership of execution tree.
     */
    WorkflowInstance(const WorkflowInstance&) = delete;

    /**
     * @brief Deleted copy assignment operator to preserve tree integrity.
     */
    WorkflowInstance& operator=(const WorkflowInstance&) = delete;

    /**
     * @brief Gets the unique identifier of this workflow instance.
     * @return std::string The instance ID.
     */
    std::string getInstanceID() const;

    /**
     * @brief Gets the definition blueprint this instance was created from.
     * @return WorkflowDefinition* Pointer to the originating WorkflowDefinition.
     */
    WorkflowDefinition* getDefinition() const;

    /**
     * @brief Gets the definition version number at the time of creation.
     * @return int The version number.
     */
    int getDefinitionVersion() const;

    /**
     * @brief Gets the root WorkItem of the cloned execution tree.
     * @return WorkItem* Pointer to the instance's root WorkItem.
     */
    WorkItem* getRoot() const;

    /**
     * @brief Gets the current lifecycle execution status.
     * @return WorkflowStatus The active workflow status.
     */
    WorkflowStatus getStatus() const;

    /**
     * @brief Gets the termination outcome or recorded reason.
     * @return std::string The recorded outcome text.
     */
    std::string getOutcome() const;

    /**
     * @brief Sets or updates the recorded outcome description.
     * @param[in] outcome String describing the instance outcome.
     */
    void setOutcome(const std::string& outcome);

    /**
     * @brief Gets the instance-specific EventPublisher.
     * @return EventPublisher* Pointer to the EventPublisher.
     */
    EventPublisher* getPublisher() const;

    /**
     * @brief Starts the workflow instance, transitioning from CREATED to RUNNING.
     * @throws std::logic_error If the instance has already been started.
     */
    void start();

    /**
     * @brief Progresses the workflow tree after a task completes.
     * @details If all items are complete, marks the instance as COMPLETED;
     * otherwise, triggers child activation for available work items.
     * Has no effect if the status is not RUNNING.
     */
    void activateNext();

    /**
     * @brief Aborts all active items and transitions instance to CANCELLED.
     * @throws std::logic_error If the instance is already finished.
     */
    void cancel();

    /**
     * @brief Halts all execution, records a failure reason, and marks as FAILED.
     * @param[in] reason Explanation describing the failure condition.
     * @throws std::logic_error If the instance is already finished.
     */
    void fail(const std::string& reason);

    /**
     * @brief Checks if the workflow instance has reached a terminal state.
     * @return true If status is COMPLETED, CANCELLED, or FAILED; false otherwise.
     */
    bool isFinished() const;

    /**
     * @brief Retrieves all work items currently eligible for assignment.
     * @details Uses an AvailableItemsIterator to traverse the composite hierarchy.
     * @return std::vector<WorkItem*> Vector of pointers to available WorkItems.
     */
    std::vector<WorkItem*> getAvailableItems() const;

private:
    /**
     * @brief Helper that finalizes the instance and notifies attached observers.
     * @param[in] finalStatus The target terminal WorkflowStatus.
     * @param[in] defaultOutcome Fallback outcome message if none is recorded.
     */
    void finish(WorkflowStatus finalStatus, const std::string& defaultOutcome);

    /**
     * @brief Attaches the instance's EventPublisher to every node in the tree.
     */
    void connectPublisher();

    std::string instanceID;            ///< Unique identifier for the instance.
    WorkflowDefinition* definition;    ///< Non-owning pointer to the originating definition.
    WorkItem* rootWorkItem;            ///< Cloned root node of the composite work item tree.
    WorkflowStatus status;             ///< Current execution lifecycle status.
    std::string outcome;               ///< Summary message or terminal outcome explanation.
    EventPublisher* publisher;         ///< Publisher broadcasting instance events to observers.
};

#endif