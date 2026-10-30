
#ifndef IWORKFLOWSERVICE_H
#define IWORKFLOWSERVICE_H

#include <string>
#include "ApprovalStatus.h"

/**
 * @brief Interface defining the workflow operations available to clients.
 *
 * The SecureWorkflowProxy implements this interface and checks permissions
 * before forwarding operations to TIVIDYFacade.
 */
class IWorkflowService {

public:

    virtual ~IWorkflowService() {}

    virtual void startInstance(
        const std::string& instanceID,
        const std::string& userID) = 0;

    virtual void assignTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& participantID,
        const std::string& userID) = 0;

    virtual void startTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID) = 0;

    virtual void completeTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID) = 0;

    virtual void rejectTask(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& reason,
        const std::string& userID) = 0;

    virtual ApprovalStatus requestApproval(
        const std::string& instanceID,
        const std::string& taskID,
        const std::string& userID) = 0;
};

#endif
