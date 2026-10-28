/**
 * @file TIVIDYFacade.cpp
 * @brief Implementation of the TIVIDYFacade class coordinating workflow execution and patterns.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-24
 */

#include "../include/TIVIDYFacade.h"

#include <cstdlib>
#include <ctime>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "../include/ApprovalHandler.h"
#include "../include/ApprovalRequest.h"
#include "../include/AssignCommand.h"
#include "../include/CompleteCommand.h"
#include "../include/EngineRouter.h"
#include "../include/EventPublisher.h"
#include "../include/Participant.h"
#include "../include/RejectCommand.h"
#include "../include/StartCommand.h"
#include "../include/WorkItem.h"
#include "../include/WorkItemIterator.h"
#include "../include/WorkflowDefinition.h"
#include "../include/WorkflowInstance.h"


namespace {

    /** 
    *@brief A task states the authority it needs through its data payload.  If the key is missing:the first approval level is assumed.
    */
    
    const char* const APPROVAL_LEVEL_KEY = "requiredApprovalLevel";
    const int DEFAULT_APPROVAL_LEVEL = 1;
    const int APPROVAL_WINDOW_SECONDS = 3600;
}

/**
 * @brief Default constructor initializing the instance ID counter to zero.
 */
TIVIDYFacade::TIVIDYFacade() : nextInstanceNumber(0) {}

/**
 * @brief Destructor deallocating routers, instances, definitions, and participants.
 */
TIVIDYFacade::~TIVIDYFacade() {
    /* The routers point at instances, so they go first. Instances hold only
    non-owning pointers to participants, so participants go last.*/

    for (auto& router : routers) {
        delete router;
    }
    routers.clear();

    for (auto& entry : instances) {
        delete entry.second;
    }
    instances.clear();

    for (auto& entry : definitions) {
        delete entry.second;
    }
    definitions.clear();

    for (auto& entry : participants) {
        delete entry.second;
    }
    participants.clear();
}

/**
 * @brief Registers and freezes a WorkflowDefinition.
 * @param[in] definition Pointer to the definition being registered.
 * @throws std::invalid_argument If definition is null or already exists.
 */
void TIVIDYFacade::registerDefinition(WorkflowDefinition* definition) {
    if (definition == nullptr) {
        throw std::invalid_argument("registerDefinition: definition is null");
    }

    const std::string name = definition->getName();
    if (definitions.count(name) > 0) {
        throw std::invalid_argument("registerDefinition: '" + name + "' is already registered");
    }

    // A definition must not change once instances can be created from it.
    if (!definition->isFrozen()) {
        definition->freeze();
    }

    definitions[name] = definition;
}

/**
 * @brief Registers a system participant with the facade.
 * @param[in] participant Pointer to the participant to register.
 * @throws std::invalid_argument If participant is null or participant ID already registered.
 */
void TIVIDYFacade::registerParticipant(Participant* participant) {
    if (participant == nullptr) {
        throw std::invalid_argument("registerParticipant: participant is null");
    }
    const std::string id = participant->getParticipantID();
    if (participants.count(id) > 0) {
        throw std::invalid_argument("registerParticipant: '" + id + "' is already registered");
    }
    participants[id] = participant;
}

/**
 * @brief Creates a cloned workflow instance using the prototype pattern.
 * @param[in] definitionName Name of the template workflow definition.
 * @return std::string The unique ID string for the created instance.
 * @throws std::invalid_argument If definitionName is not registered.
 */
std::string TIVIDYFacade::createInstance(const std::string& definitionName) {
    auto found = definitions.find(definitionName);
    if (found == definitions.end()) {
        throw std::invalid_argument("createInstance: unknown definition '" + definitionName + "'");
    }
    WorkflowDefinition* definition = found->second;

    std::ostringstream idStream;
    idStream << "INST-" << ++nextInstanceNumber;
    const std::string instanceID = idStream.str();


    // Prototype: the definition clones its tree into a new, independent instance.
    WorkflowInstance* instance = definition->createInstance(instanceID);
    instances[instanceID] = instance;


    // Observer: attach every listener so each state change reaches all of them.
    EventPublisher* publisher = instance->getPublisher();
    publisher->attach(&audit);
    publisher->attach(&notifier);
    publisher->attach(&metrics);

    EngineRouter* router = new EngineRouter(instance, definition->getApprovalChain());
    routers.push_back(router);
    publisher->attach(router);

    return instanceID;
}

/**
 * @brief Starts execution of the specified workflow instance.
 * @param[in] instanceID The unique ID of the instance to run.
 */
void TIVIDYFacade::startInstance(const std::string& instanceID) {
    findInstance(instanceID)->start();
}

/**
 * @brief Creates and executes an AssignCommand for a work item.
 * @param[in] instanceID Target workflow instance ID.
 * @param[in] taskID Target task work item ID.
 * @param[in] participantID Target participant ID.
 */
void TIVIDYFacade::assignTask(const std::string& instanceID, const std::string& taskID,
                              const std::string& participantID) {
    // Look everything up first so a bad ID never leaves a half-built command behind.
    WorkItem* item = findItem(instanceID, taskID);
    Participant* participant = findParticipant(participantID);
    invoker.executeCommand(new AssignCommand(item, participant));
}

/**
 * @brief Creates and executes a CompleteCommand for a work item.
 * @param[in] instanceID Target workflow instance ID.
 * @param[in] taskID Target task work item ID.
 */
void TIVIDYFacade::completeTask(const std::string& instanceID, const std::string& taskID) {
    WorkItem* item = findItem(instanceID, taskID);
    invoker.executeCommand(new CompleteCommand(item));
}

/**
 * @brief Creates and executes a RejectCommand for a work item.
 * @param[in] instanceID Target workflow instance ID.
 * @param[in] taskID Target task work item ID.
 * @param[in] reason Reason for task rejection.
 */
void TIVIDYFacade::rejectTask(const std::string& instanceID, const std::string& taskID,
                              const std::string& reason) {
    WorkItem* item = findItem(instanceID, taskID);
    invoker.executeCommand(new RejectCommand(item, reason));
}

/**
 * @brief Delegates undo execution to the CommandInvoker.
 * @return true If a command was successfully undone; false otherwise.
 */
bool TIVIDYFacade::undoLastAction() {
    return invoker.undoLast();
}

/**
 * @brief Submits a work item to an approval chain of responsibility.
 * @param[in] instanceID The unique instance identifier.
 * @param[in] taskID The unique task identifier.
 * @return ApprovalStatus The final status of the approval request.
 * @throws std::runtime_error If the instance's definition has no approval chain.
 */
ApprovalStatus TIVIDYFacade::requestApproval(const std::string& instanceID,
                                             const std::string& taskID) {
    WorkflowInstance* instance = findInstance(instanceID);
    WorkItem* item = findItem(instanceID, taskID);

    ApprovalHandler* chain = instance->getDefinition()->getApprovalChain();
    if (chain == nullptr) {
        throw std::runtime_error("requestApproval: the definition has no approval chain");
    }

    int requiredLevel = DEFAULT_APPROVAL_LEVEL;
    if (item->hasData(APPROVAL_LEVEL_KEY)) {
        requiredLevel = std::atoi(item->getData(APPROVAL_LEVEL_KEY).c_str());
    }

    const std::time_t deadline = std::time(nullptr) + APPROVAL_WINDOW_SECONDS;
    ApprovalRequest request(item, requiredLevel, deadline);

    // Chain of Responsibility: each level decides or forwards to the next.
    chain->handle(&request);
    return request.getStatus();
}

/**
 * @brief Returns the summary from the MetricsCollector listener.
 * @return std::string Metrics summary text.
 */
std::string TIVIDYFacade::getMetricsSummary() {
    return metrics.getSummary();
}

/**
 * @brief Returns logged events recorded by the AuditLogger.
 * @return std::vector<std::string> Audit log entries.
 */
std::vector<std::string> TIVIDYFacade::getAuditLog() {
    return audit.getRecords();
}

/**
 * @brief Locates a WorkItem using the Iterator pattern.
 * @param[in] instanceID The identifier of the instance to search.
 * @param[in] itemID The identifier of the WorkItem to find.
 * @return WorkItem* Pointer to the resolved WorkItem.
 * @throws std::invalid_argument If the item does not exist in the instance tree.
 */
WorkItem* TIVIDYFacade::findItem(const std::string& instanceID, const std::string& itemID) {
    WorkflowInstance* instance = findInstance(instanceID);

    // Iterator: walk the tree without knowing how stages store their children.
    std::unique_ptr<WorkItemIterator> it(instance->getRoot()->createIterator());
    for (it->first(); !it->isDone(); it->next()) {
        WorkItem* item = it->current();
        if (item->getItemID() == itemID) {
            return item;
        }
    }
    throw std::invalid_argument("findItem: no item '" + itemID + "' in instance '" + instanceID + "'");
}

/**
 * @brief Helper lookup for registered workflow instances.
 * @param[in] instanceID Unique instance identifier.
 * @return WorkflowInstance* Pointer to the found WorkflowInstance.
 * @throws std::invalid_argument If instanceID is not recognized.
 */
WorkflowInstance* TIVIDYFacade::findInstance(const std::string& instanceID) {
    auto found = instances.find(instanceID);
    if (found == instances.end()) {
        throw std::invalid_argument("unknown instance '" + instanceID + "'");
    }
    return found->second;
}

/**
 * @brief Helper lookup for registered participants.
 * @param[in] participantID Unique participant identifier.
 * @return Participant* Pointer to the found Participant.
 * @throws std::invalid_argument If participantID is not recognized.
 */
Participant* TIVIDYFacade::findParticipant(const std::string& participantID) {
    auto found = participants.find(participantID);
    if (found == participants.end()) {
        throw std::invalid_argument("unknown participant '" + participantID + "'");
    }
    return found->second;
}

/**
 * @brief Creates and executes a StartCommand for a work item.
 * @param[in] instanceID Target workflow instance ID.
 * @param[in] taskID Target task work item ID.
 */
void TIVIDYFacade::startTask(const std::string& instanceID, const std::string& taskID) {
    WorkItem* item = findItem(instanceID, taskID);
    invoker.executeCommand(new StartCommand(item));
}

/**
  * @brief Creates and executes a CompleteCommand for a work item.
  * @param[in] instanceID Target workflow instance ID.
  */