#ifndef APPROVALSTATUS_H
#define APPROVALSTATUS_H

/**
 * @brief The possible states of an ApprovalRequest.
 */
enum class ApprovalStatus {
    Pending,    ///< Waiting for a decision.
    Approved,   ///< Approved by a level with enough authority.
    Rejected,   ///< Rejected by an approver.
    Escalated   ///< Passed on to a higher approval level.
};

#endif