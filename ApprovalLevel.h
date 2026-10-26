#ifndef APPROVALLEVEL_H
#define APPROVALLEVEL_H

#include "ApprovalHandler.h"

class Role;

class ApprovalLevel : public ApprovalHandler {

    private:

        Role* role;
        
        int timeoutSeconds;

    public:

        ApprovalLevel(Role* role, int timeoutSeconds);

        ~ApprovalLevel();

        bool canApprove(const ApprovalRequest* request);

        void handle(ApprovalRequest* request) override;

        Role* getRole();

        int getTimeout();
};
#endif