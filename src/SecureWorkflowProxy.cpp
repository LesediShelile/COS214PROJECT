
#ifndef SECUREWORKFLOWPROXY_H
#define SECUREWORKFLOWPROXY_H

#include <string>

#include "IWorkflowService.h"

class TIVIDYFacade;

/**
 * @brief Protection Proxy that controls access to TIVIDY workflow operations.
 *
 * Checks participant permissions and task ownership before forwarding
 * requests to the actual TIVIDYFacade.
 *
 * The facade is not owned by this class.
 */
class SecureWorkflowProxy : public IWorkflowService {

private:

    /** @brief The actual workflow service receiving authorised requests. */
    TIVIDYFacade* realService;

    /**
     * @brief Checks whether a participant has a specific permission.
     * @return true if the participant has the permission.
     */
    bool hasPermission(
        const std::string& userID,
        const std::string& permission);

    /**
     * @brief Checks whether a participant is assigned to a task.
     * @return true if the participant owns the assignment.
     */
    bool ownsTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID);

    /**
     * @brief Rejects an operation if the participant lacks permission.
     */
    void requirePermission(
        const std::string& userID,
        const std::string& permission);

    /**
     * @brief Rejects an operation if the participant is not assigned to the task.
     */
    void requireOwnership(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID);

public:

    /**
     * @brief Creates a security proxy around the actual workflow service.
     * @param service Pointer to the existing TIVIDYFacade.
     */
    explicit SecureWorkflowProxy(TIVIDYFacade* service);

    virtual ~SecureWorkflowProxy();

    virtual void startInstance(
        const std::string& instanceID,
        const std::string& userID);

    virtual void assignTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& participantID,
        const std::string& userID);

    virtual void startTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID);

    virtual void completeTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID);

    virtual void rejectTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& reason,
        const std::string& userID);

    virtual ApprovalStatus requestApproval(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID);
};

#endif
