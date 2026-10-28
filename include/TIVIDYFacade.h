/**
 * @file TIVIDYFacade.h
 * @brief Header file declaring the TIVIDYFacade class for the TIVIDY workflow engine.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-24
 */

#ifndef TIVIDYFACADE_H
#define TIVIDYFACADE_H

#include <map>
#include <string>
#include <vector>

#include "../include/ApprovalRequest.h"
#include "../include/AuditLogger.h"
#include "../include/CommandInvoker.h"
#include "../include/MetricsCollector.h"
#include "../include/Notifier.h"

class WorkflowDefinition;
class WorkflowInstance;
class WorkItem;
class Participant;
class EngineRouter;

/**
 * @class TIVIDYFacade
 * @brief Facade providing a unified entry point to the TIVIDY workflow engine subsystem.
 * 
 * Clients register definitions and participants, create and start instances,
 * and act on work items by ID. They never touch the underlying builders, iterators,
 * commands, listeners, or approval chains directly.
 * 
 * The facade manages the lifecycle and owns every definition, instance, participant,
 * and router registered with or created by it. Listeners (audit, notifier, metrics)
 * are members, and instances hold non-owning references to them.
 * 
 * @see WorkflowDefinition
 * @see WorkflowInstance
 * @see CommandInvoker
 */
class TIVIDYFacade {
public:
    /**
     * @brief Constructs a new TIVIDYFacade instance.
     * Initializes the instance numbering sequence.
     */
    TIVIDYFacade();

    /**
     * @brief Destructor that deallocates all managed resources.
     * Cleans up routers, instances, definitions, and participants in order of dependency.
     */
    ~TIVIDYFacade();

    /**
     * @brief Deleted copy constructor to prevent shallow copies of managed pointers.
     */
    TIVIDYFacade(const TIVIDYFacade&) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent multiple ownership.
     */
    TIVIDYFacade& operator=(const TIVIDYFacade&) = delete;

    /**
     * @brief Registers a workflow definition with the facade.
     * @details Freezes the definition to prevent modification and transfers ownership
     * to the facade.
     * @param[in] definition Pointer to the WorkflowDefinition to register.
     * @throws std::invalid_argument If definition is null or already registered.
     */
    void registerDefinition(WorkflowDefinition* definition);

    /**
     * @brief Registers a participant eligible to be assigned work items.
     * @param[in] participant Pointer to the Participant to register.
     * @throws std::invalid_argument If participant is null or participant ID is already registered.
     */
    void registerParticipant(Participant* participant);

    /**
     * @brief Instantiates a new workflow from a registered definition.
     * @details Clones the prototype workflow definition hierarchy and wires all
     * default observers (AuditLogger, Notifier, MetricsCollector, and EngineRouter).
     * @param[in] definitionName The unique name of the registered WorkflowDefinition.
     * @return std::string The generated unique identifier for the new workflow instance.
     * @throws std::invalid_argument If definitionName is not recognized.
     */
    std::string createInstance(const std::string& definitionName);

    /**
     * @brief Starts execution of an existing workflow instance.
     * @param[in] instanceID The unique identifier of the instance to start.
     * @throws std::invalid_argument If instanceID does not match an existing instance.
     */
    void startInstance(const std::string& instanceID);

    /**
     * @brief Assigns a task within an instance to a specific participant.
     * @details Executed as a Command via CommandInvoker to support undo functionality.
     * @param[in] instanceID The unique identifier of the workflow instance.
     * @param[in] taskID The unique identifier of the task item to assign.
     * @param[in] participantID The unique identifier of the participant.
     * @throws std::invalid_argument If instanceID, taskID, or participantID is not found.
     */
    void assignTask(const std::string& instanceID, const std::string& taskID,
                    const std::string& participantID);

    /**
     * @brief Marks a specific task item within an instance as completed.
     * @param[in] instanceID The unique identifier of the workflow instance.
     * @param[in] taskID The unique identifier of the task item to complete.
     * @throws std::invalid_argument If instanceID or taskID is not found.
     */
    void completeTask(const std::string& instanceID, const std::string& taskID);

    /**
     * @brief Rejects a specific task item within an instance with a stated reason.
     * @param[in] instanceID The unique identifier of the workflow instance.
     * @param[in] taskID The unique identifier of the task item to reject.
     * @param[in] reason Explanation describing why the task was rejected.
     * @throws std::invalid_argument If instanceID or taskID is not found.
     */
    void rejectTask(const std::string& instanceID, const std::string& taskID,
                    const std::string& reason);

    /**
     * @brief Undoes the most recently executed command.
     * @return true If an action was successfully undone, false if the undo stack is empty.
     */
    bool undoLastAction();

    /**
     * @brief Initiates an approval evaluation for a specific task using the approval chain.
     * @param[in] instanceID The unique identifier of the workflow instance.
     * @param[in] taskID The unique identifier of the task requiring approval.
     * @return ApprovalStatus The final decision status after propagating through the chain.
     * @throws std::invalid_argument If instanceID or taskID is not found.
     * @throws std::runtime_error If the workflow definition does not have an approval chain configured.
     */
    ApprovalRequest requestApproval(const std::string& instanceID,
                                   const std::string& taskID);

    /**
     * @brief Retrieves the aggregate performance and execution metrics summary.
     * @return std::string Formatted metrics summary text.
     */
    std::string getMetricsSummary();

    /**
     * @brief Retrieves the complete audit log records collected across all instances.
     * @return std::vector<std::string> A list of logged event record strings.
     */
    std::vector<std::string> getAuditLog();

private:
    /**
     * @brief Searches for a work item within a specific workflow instance hierarchy.
     * @details Uses an iterator to traverse composite work item structures.
     * @param[in] instanceID The unique identifier of the workflow instance.
     * @param[in] itemID The unique identifier of the work item to find.
     * @return WorkItem* Pointer to the matching WorkItem.
     * @throws std::invalid_argument If the instance does not exist or itemID is not present.
     */
    WorkItem* findItem(const std::string& instanceID, const std::string& itemID);

    /**
     * @brief Looks up a workflow instance by its unique identifier.
     * @param[in] instanceID The unique identifier of the workflow instance.
     * @return WorkflowInstance* Pointer to the found WorkflowInstance.
     * @throws std::invalid_argument If instanceID is not registered.
     */
    WorkflowInstance* findInstance(const std::string& instanceID);

    /**
     * @brief Looks up a participant by their unique identifier.
     * @param[in] participantID The unique identifier of the participant.
     * @return Participant* Pointer to the found Participant.
     * @throws std::invalid_argument If participantID is not registered.
     */
    Participant* findParticipant(const std::string& participantID);

    std::map<std::string, WorkflowDefinition*> definitions; ///< Registered workflow definitions keyed by name.
    std::map<std::string, WorkflowInstance*> instances;     ///< Active and finished workflow instances keyed by ID.
    std::map<std::string, Participant*> participants;       ///< Registered system participants keyed by ID.
    std::vector<EngineRouter*> routers;                     ///< Dynamic event routers created for instances.
    CommandInvoker invoker;                                 ///< Invoker executing and tracking undoable actions.
    AuditLogger audit;                                      ///< Global audit logger listening to workflow events.
    Notifier notifier;                                      ///< Global notification dispatcher.
    MetricsCollector metrics;                               ///< Aggregator tracking execution metrics.
    int nextInstanceNumber;                                 ///< Monotonically increasing counter for instance ID generation.
};

#endif