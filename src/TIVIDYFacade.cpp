#include "TIVIDYFacade.h"

#include <cstdlib>
#include <ctime>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "ApprovalHandler.h"
#include "ApprovalRequest.h"
#include "AssignCommand.h"
#include "CompleteCommand.h"
#include "EngineRouter.h"
#include "EventPublisher.h"
#include "Participant.h"
#include "RejectCommand.h"
#include "WorkItem.h"
#include "WorkItemIterator.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"

//Kiara Ajodhaparsadh

namespace {
    // A task states the authority it needs through its data payload.
    // If the key is missing:the first approval level is assumed.

    const char* const APPROVAL_LEVEL_KEY = "requiredApprovalLevel";
    const int DEFAULT_APPROVAL_LEVEL = 1;
    const int APPROVAL_WINDOW_SECONDS = 3600;
}

TIVIDYFacade::TIVIDYFacade() : nextInstanceNumber(0) {}

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


void TIVIDYFacade::startInstance(const std::string& instanceID) {
    findInstance(instanceID)->start();
}


void TIVIDYFacade::assignTask(const std::string& instanceID, const std::string& taskID,
                              const std::string& participantID) {
    // Look everything up first so a bad ID never leaves a half-built command behind.
    WorkItem* item = findItem(instanceID, taskID);
    Participant* participant = findParticipant(participantID);
    invoker.executeCommand(new AssignCommand(item, participant));
}


void TIVIDYFacade::completeTask(const std::string& instanceID, const std::string& taskID) {
    WorkItem* item = findItem(instanceID, taskID);
    invoker.executeCommand(new CompleteCommand(item));
}


void TIVIDYFacade::rejectTask(const std::string& instanceID, const std::string& taskID,
                              const std::string& reason) {
    WorkItem* item = findItem(instanceID, taskID);
    invoker.executeCommand(new RejectCommand(item, reason));
}


bool TIVIDYFacade::undoLastAction() {
    return invoker.undoLast();
}


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

std::string TIVIDYFacade::getMetricsSummary() {
    return metrics.getSummary();
}

std::vector<std::string> TIVIDYFacade::getAuditLog() {
    return audit.getRecords();
}

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

WorkflowInstance* TIVIDYFacade::findInstance(const std::string& instanceID) {
    auto found = instances.find(instanceID);
    if (found == instances.end()) {
        throw std::invalid_argument("unknown instance '" + instanceID + "'");
    }
    return found->second;
}

Participant* TIVIDYFacade::findParticipant(const std::string& participantID) {
    auto found = participants.find(participantID);
    if (found == participants.end()) {
        throw std::invalid_argument("unknown participant '" + participantID + "'");
    }
    return found->second;
}