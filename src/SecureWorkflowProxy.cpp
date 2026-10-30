
#include "../include/SecureWorkflowProxy.h"

#include "../include/TIVIDYFacade.h"
#include "../include/Participant.h"
#include "../include/Role.h"

#include <stdexcept>

/**
 * @brief Creates the security proxy.
 * @param service The actual TIVIDY workflow facade.
 */
SecureWorkflowProxy::SecureWorkflowProxy(TIVIDYFacade* service)
    : realService(service) {

    if (realService == nullptr) {
        throw std::invalid_argument(
            "SecureWorkflowProxy: service cannot be null");
    }
}

/**
 * @brief Destructor.
 *
 * The proxy does not own the facade.
 */
SecureWorkflowProxy::~SecureWorkflowProxy() {}

/**
 * @brief Checks whether a participant has a specific permission.
 */
bool SecureWorkflowProxy::hasPermission(
    const std::string& userID,
    const std::string& permission) {

    try {

        const Participant* user =
            realService->getParticipant(userID);

        if (user == nullptr || user->getRole() == nullptr) {
            return false;
        }

        return user->getRole()->hasPermission(permission);

    }
    catch (const std::invalid_argument&) {

        return false;

    }
}

/**
 * @brief Checks whether a participant owns a particular task.
 */
bool SecureWorkflowProxy::ownsTask(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& userID) {

    return realService->isTaskAssignedTo(
        instanceID, taskID, userID);
}

/**
 * @brief Checks permission and rejects unauthorised operations.
 */
void SecureWorkflowProxy::requirePermission(
    const std::string& userID,
    const std::string& permission) {

    if (!hasPermission(userID, permission)) {

        throw std::runtime_error(
            "Access denied: User '" + userID +
            "' does not have permission '" + permission + "'");

    }
}

/**
 * @brief Checks task ownership.
 */
void SecureWorkflowProxy::requireOwnership(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& userID) {

    if (!ownsTask(instanceID, taskID, userID)) {

        throw std::runtime_error(
            "Access denied: User '" + userID +
            "' is not assigned to task '" + taskID + "'");

    }
}

/**
 * @brief Starts a workflow instance after checking permission.
 */
void SecureWorkflowProxy::startInstance(
    const std::string& instanceID,
    const std::string& userID) {

    requirePermission(userID, "workflow.start");

    realService->startInstance(instanceID);
}

/**
 * @brief Assigns a task after checking permission.
 */
void SecureWorkflowProxy::assignTask(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& participantID,
    const std::string& userID) {

    requirePermission(userID, "task.assign");

    realService->assignTask(
        instanceID, taskID, participantID);
}

/**
 * @brief Starts a task after checking permission and ownership.
 */
void SecureWorkflowProxy::startTask(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& userID) {

    requirePermission(userID, "task.start");

    requireOwnership(instanceID, taskID, userID);

    realService->startTask(instanceID, taskID);
}

/**
 * @brief Completes a task after checking permission and ownership.
 */
void SecureWorkflowProxy::completeTask(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& userID) {

    requirePermission(userID, "task.complete");

    requireOwnership(instanceID, taskID, userID);

    realService->completeTask(instanceID, taskID);
}

/**
 * @brief Rejects a task after checking permission and ownership.
 */
void SecureWorkflowProxy::rejectTask(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& reason,
    const std::string& userID) {

    requirePermission(userID, "task.reject");

    requireOwnership(instanceID, taskID, userID);

    realService->rejectTask(instanceID, taskID, reason);
}

/**
 * @brief Requests approval after checking permission and ownership.
 */
ApprovalStatus SecureWorkflowProxy::requestApproval(
    const std::string& instanceID,
    const std::string& taskID,
    const std::string& userID) {

    requirePermission(userID, "approval.request");

    requireOwnership(instanceID, taskID, userID);

    return realService->requestApproval(instanceID, taskID);
}
