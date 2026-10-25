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

WorkflowInstance::~WorkflowInstance() {
    delete rootWorkItem;
    delete publisher;
}

std::string WorkflowInstance::getInstanceID() const {
    return instanceID;
}

WorkflowDefinition* WorkflowInstance::getDefinition() const {
    return definition;
}

int WorkflowInstance::getDefinitionVersion() const {
    return definition->getVersion();
}

WorkItem* WorkflowInstance::getRoot() const {
    return rootWorkItem;
}

WorkflowStatus WorkflowInstance::getStatus() const {
    return status;
}

std::string WorkflowInstance::getOutcome() const {
    return outcome;
}

void WorkflowInstance::setOutcome(const std::string& newOutcome) {
    outcome = newOutcome;
}

EventPublisher* WorkflowInstance::getPublisher() const {
    return publisher;
}

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

void WorkflowInstance::cancel() {
    if (isFinished()) {
        throw std::logic_error("cancel: instance '" + instanceID + "' is already finished");
    }
    rootWorkItem->cancel();
    finish(WorkflowStatus::CANCELLED, "Cancelled");
}

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

bool WorkflowInstance::isFinished() const {
    return status == WorkflowStatus::COMPLETED ||
           status == WorkflowStatus::CANCELLED ||
           status == WorkflowStatus::FAILED;
}

std::vector<WorkItem*> WorkflowInstance::getAvailableItems() const {
    std::vector<WorkItem*> available;
    // Iterator: walk the tree without knowing how stages store their children.
    std::unique_ptr<WorkItemIterator> it(rootWorkItem->createAvailableIterator());
    for (it->first(); !it->isDone(); it->next()) {
        available.push_back(it->current());
    }
    return available;
}

void WorkflowInstance::finish(WorkflowStatus finalStatus, const std::string& defaultOutcome) {
    status = finalStatus;
    if (outcome.empty()) {
        outcome = defaultOutcome;
    }
    // Observer: tell every listener the workflow has ended.
    WorkflowEvent event(EventType::WORKFLOW_FINISHED, instanceID, currentTimestamp(), outcome);
    publisher->publish(&event);
}

void WorkflowInstance::connectPublisher() {
    // Give every item in the cloned tree this instance's publisher.

    std::unique_ptr<WorkItemIterator> it(rootWorkItem->createIterator());
    for (it->first(); !it->isDone(); it->next()) {
        it->current()->setPublisher(publisher);
    }
}