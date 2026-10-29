#include "ApprovalLevel.h"
#include "ApprovalRequest.h"
#include "Role.h"
#include <ctime>
#include <iostream>

ApprovalLevel::ApprovalLevel(Role* role, int timeoutSeconds) : role(role), timeoutSeconds(timeoutSeconds) {}

// The level does not own the role, so nothing to delete
ApprovalLevel::~ApprovalLevel() {}

bool ApprovalLevel::canApprove(const ApprovalRequest* request) {

    if (role == nullptr) {

        return false;

    }

    // Already approved or rejected: nothing left to decide
    ApprovalStatus status = request->getStatus();

    if (status == ApprovalStatus::Approved || status == ApprovalStatus::Rejected) {

        return false;

    }

    return role->getAuthorityLevel() >= request->getRequiredLevel();
}

void ApprovalLevel::handle(ApprovalRequest* request) {

    // Already approved or rejected: stop here
    ApprovalStatus status = request->getStatus();
    
    if (status == ApprovalStatus::Approved || status == ApprovalStatus::Rejected) {

        std::cout << "Request has already been decided" << std::endl;

        return;

    }

    // This level gets its own time window to decide
    request->resetDeadline(time(nullptr) + timeoutSeconds);

    if (canApprove(request)) {

        request->approve(role);

        std::cout << role->getName() << " approved the request" << std::endl;

    } else {

        std::cout << (role ? role->getName() : "Unknown role") << " cannot approve, passing it on" << std::endl;

        request->markEscalated();

        forward(request);
    }
}

Role* ApprovalLevel::getRole() {

    return role;

}

int ApprovalLevel::getTimeout() {

    return timeoutSeconds;
    
}