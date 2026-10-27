#include "ApprovalRequest.h"
#include <iostream>

ApprovalRequest ::  ApprovalRequest (WorkItem * subject, int requiredLevel, time_t deadline) : subject(subject), requiredLevel(requiredLevel), status(ApprovalStatus::Pending), createdAt(time(nullptr)), deadline(deadline), decidedBy(nullptr) {}

ApprovalRequest :: ~ApprovalRequest() {}

WorkItem* ApprovalRequest :: getSubject() const{

    return subject;

}

int ApprovalRequest :: getRequiredLevel() const{

    return requiredLevel;

}

ApprovalStatus ApprovalRequest :: getStatus() const{

    return status;

}

time_t ApprovalRequest :: getDeadline() const{

    return deadline;

}

void ApprovalRequest :: resetDeadline (time_t deadline) {

    this -> deadline = deadline;

}

void ApprovalRequest :: approve (Role* by){

    if (status == ApprovalStatus::Approved || status == ApprovalStatus::Rejected) {

        std::cout << "Request has already been decided" << std::endl;

        return;
    }
    status = ApprovalStatus::Approved;

    decidedBy = by;
}

void ApprovalRequest :: reject (Role* by){

    if (status == ApprovalStatus::Approved || status == ApprovalStatus::Rejected){

        std::cout << "Request has already been decided" << std::endl;

        return;
    }
    status = ApprovalStatus::Rejected;

    decidedBy = by;
}

void ApprovalRequest :: markEscalated() {

    if (status == ApprovalStatus :: Pending){
        
        status = ApprovalStatus::Escalated;
    }
}

bool ApprovalRequest :: isExpired(time_t now) const{

    bool undecided = status == ApprovalStatus::Pending || status == ApprovalStatus::Escalated;

    return undecided && now > deadline;
}
  