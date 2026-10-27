#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H


#include <iostream>
#include <ctime>

#include "ApprovalStatus.h"

class WorkItem;
class Role;

/**
 * @brief A request for approval that is passed along the approval chain.
 *
 * Records what needs approving, the authority level required,
 * the deadline and the final decision.
 */
class ApprovalRequest {

    private:

        /** @brief The work item that needs approval (not owned). */
        WorkItem* subject;

        /** @brief The minimum authority level needed to approve. */
        int requiredLevel;

        /** @brief The current status of the request. */
        ApprovalStatus status;

        /** @brief When the request was created. */
        time_t createdAt;

        /** @brief When the current approval level must decide by. */
        time_t deadline;

        /** @brief The role that approved or rejected the request (not owned). */
        Role* decidedBy;


    public:

        /**
         * @brief Creates a pending approval request.
         * @param subject The work item that needs approval.
         * @param requiredLevel The minimum authority level needed to approve.
         * @param deadline When the request must be decided by.
         */
        ApprovalRequest(WorkItem* subject, int requiredLevel, time_t deadline);

        /** @brief Destructor. The subject and role are not owned, so they are not deleted. */
        ~ApprovalRequest();

        /**
         * @brief Gets the work item that needs approval.
         * @return The subject of the request.
         */
        WorkItem* getSubject() const;

        /**
         * @brief Gets the authority level needed to approve.
         * @return The required level.
         */
        int getRequiredLevel() const;

        /**
         * @brief Gets the current status of the request.
         * @return Pending, Approved, Rejected or Escalated.
         */
        ApprovalStatus getStatus() const;

        /**
         * @brief Gets the current deadline.
         * @return The deadline.
         */
        time_t getDeadline() const;

        /**
         * @brief Sets a new deadline, e.g. when the request moves to the next level.
         * @param deadline The new deadline.
         */
        void resetDeadline (time_t deadline);

        /**
         * @brief Approves the request. Does nothing if it has already been decided.
         * @param by The role that approved it.
         */
        void approve (Role* by);

        /**
         * @brief Rejects the request. Does nothing if it has already been decided.
         * @param by The role that rejected it.
         */
        void reject (Role* by);

        /** @brief Marks a pending request as escalated to a higher level. */
        void markEscalated();

        /**
         * @brief Checks whether an undecided request has passed its deadline.
         * @param now The current time.
         * @return True if the request is undecided and now is past the deadline.
         */
        bool isExpired (time_t now) const;
};
#endif