#ifndef APPROVALLEVEL_H
#define APPROVALLEVEL_H

#include "ApprovalHandler.h"

class Role;

/**
 * @brief Concrete handler in the approval chain (Chain of Responsibility).
 *
 * Approves a request if its role's authority level is high enough,
 * otherwise escalates the request to the next level.
 */
class ApprovalLevel : public ApprovalHandler {

    private:

        /** @brief The role that approves at this level. */
        Role* role;
        
        /** @brief How long this level has to decide, in seconds. */
        int timeoutSeconds;

    public:

        /**
         * @brief Creates an approval level.
         * @param role The role that approves at this level.
         * @param timeoutSeconds How long this level has to decide, in seconds.
         */
        ApprovalLevel(Role* role, int timeoutSeconds);

        /** @brief Destructor. The role is not owned, so it is not deleted. */
        ~ApprovalLevel();

        /**
         * @brief Checks whether this level is allowed to approve the request.
         * @param request The request to check.
         * @return True if the role's authority level is high enough and the request is undecided.
         */
        bool canApprove(const ApprovalRequest* request);

        /**
         * @brief Approves the request, or escalates it to the next level.
         * @param request The request to handle.
         */
        void handle(ApprovalRequest* request) override;

        /**
         * @brief Gets the role for this level.
         * @return The role that approves at this level.
         */
        Role* getRole();

        /**
         * @brief Gets how long this level has to decide.
         * @return The timeout in seconds.
         */
        int getTimeout();
};
#endif