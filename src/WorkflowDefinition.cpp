/**
 * @file WorkflowDefinition.cpp
 * @brief Implementation of the WorkflowDefinition class managing workflow blueprints.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-27
 */

#include "../include/WorkflowDefinition.h"

#include <stdexcept>

#include "../include/ApprovalHandler.h"
#include "../include/WorkItem.h"
#include "../include/WorkflowInstance.h"

/**
 * @brief Constructs a new WorkflowDefinition with zero initial handlers and unfrozen status.
 * @param[in] name The name identifier of the workflow blueprint.
 * @param[in] version The blueprint version.
 */
WorkflowDefinition::WorkflowDefinition(const std::string& name, int version)
    : name(name), version(version), root(nullptr), approvalChain(nullptr), frozen(false) {}

/**
 * @brief Deallocates the composite root and the approval handler chain.
 */
WorkflowDefinition::~WorkflowDefinition() {
    delete root;

    // Deleting the head also deletes the rest of the chain, because each ApprovalHandler owns its successor.
  
    delete approvalChain;
}

/**
 * @brief Retrieves the workflow definition name.
 * @return std::string The definition name.
 */
std::string WorkflowDefinition::getName() const {
    return name;
}

/**
 * @brief Retrieves the workflow version.
 * @return int The version number.
 */
int WorkflowDefinition::getVersion() const {
    return version;
}

/**
 * @brief Retrieves the root WorkItem.
 * @return WorkItem* Pointer to the root node.
 */
WorkItem* WorkflowDefinition::getRoot() const {
    return root;
}

/**
 * @brief Retrieves the approval handler chain head.
 * @return ApprovalHandler* Pointer to the head handler.
 */
ApprovalHandler* WorkflowDefinition::getApprovalChain() const {
    return approvalChain;
}

/**
 * @brief Assigns a new root WorkItem to an unfrozen definition.
 * @param[in] newRoot Pointer to the new root WorkItem.
 * @throws std::logic_error If the definition is frozen.
 * @throws std::invalid_argument If newRoot is null.
 */
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

/**
 * @brief Assigns a new approval chain head to an unfrozen definition.
 * @param[in] handler Pointer to the new chain head ApprovalHandler.
 * @throws std::logic_error If the definition is frozen.
 */
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

/**
 * @brief Locks the definition to make it immutable.
 * @throws std::logic_error If no root work item has been assigned.
 */
void WorkflowDefinition::freeze() {
    if (root == nullptr) {
        throw std::logic_error("freeze: definition '" + name + "' has no root work item");
    }
    frozen = true;
}

/**
 * @brief Returns whether the definition is frozen.
 * @return true If modifications are disallowed, false otherwise.
 */
bool WorkflowDefinition::isFrozen() const {
    return frozen;
}

/**
 * @brief Factory method producing a cloned WorkflowInstance using Prototype.
 * @param[in] instanceID The unique instance ID string.
 * @return WorkflowInstance* Newly allocated workflow instance.
 * @throws std::logic_error If called before freezing the definition.
 */
WorkflowInstance* WorkflowDefinition::createInstance(const std::string& instanceID) {
    if (!frozen) {
        throw std::logic_error("createInstance: definition '" + name + "' must be frozen first");
    }
    // clones the tree (Prototype).
    return new WorkflowInstance(instanceID, this);
}