#include "../include/WorkflowDefinition.h"

#include <stdexcept>

#include "../include/ApprovalHandler.h"
#include "../include/WorkItem.h"
#include "../include/WorkflowInstance.h"

WorkflowDefinition::WorkflowDefinition(const std::string& name, int version)
    : name(name), version(version), root(nullptr), approvalChain(nullptr), frozen(false) {}

WorkflowDefinition::~WorkflowDefinition() {
    delete root;

    // Deleting the head also deletes the rest of the chain, because each ApprovalHandler owns its successor.
  
    delete approvalChain;
}

std::string WorkflowDefinition::getName() const {
    return name;
}

int WorkflowDefinition::getVersion() const {
    return version;
}

WorkItem* WorkflowDefinition::getRoot() const {
    return root;
}

ApprovalHandler* WorkflowDefinition::getApprovalChain() const {
    return approvalChain;
}

void WorkflowDefinition::setRoot(WorkItem* newRoot) {
    if (frozen) {
        throw std::logic_error("setRoot: definition '" + name + "' is frozen");
    }
    if (newRoot == nullptr) {
        throw std::invalid_argument("setRoot: root is null");
    }
    if (newRoot == root) {
        return;
    }
    delete root;
    root = newRoot;
}

void WorkflowDefinition::setApprovalChain(ApprovalHandler* handler) {
    if (frozen) {
        throw std::logic_error("setApprovalChain: definition '" + name + "' is frozen");
    }
    if (handler == approvalChain) {
        return;
    }
    delete approvalChain;
    approvalChain = handler;
}

void WorkflowDefinition::freeze() {
    if (root == nullptr) {
        throw std::logic_error("freeze: definition '" + name + "' has no root work item");
    }
    frozen = true;
}

bool WorkflowDefinition::isFrozen() const {
    return frozen;
}

WorkflowInstance* WorkflowDefinition::createInstance(const std::string& instanceID) {
    if (!frozen) {
        throw std::logic_error("createInstance: definition '" + name + "' must be frozen first");
    }
    // clones the tree (Prototype).
    return new WorkflowInstance(instanceID, this);
}