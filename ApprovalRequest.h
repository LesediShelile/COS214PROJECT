#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H


#include <iostream>
#include <ctime>

#include "ApprovalStatus.h"

class WorkItem;
class Role;

class ApprovalRequest {

    private:

        WorkItem* subject;

        int requiredLevel;

        ApprovalStatus status;

        time_t createdAt;

        time_t deadline;

        Role* decidedBy;


    public:

        ApprovalRequest(WorkItem* subject, int requiredLevel, time_t deadline);

        ~ApprovalRequest();

        WorkItem* getSubject() const;

        int getRequiredLevel() const;

        ApprovalStatus getStatus() const;

        time_t getDeadline() const;

        void resetDeadline (time_t deadline);

        void approve (Role* by);

        void reject (Role* by);

        void markEscalated();

        bool isExpired (time_t now) const;
};
#endif