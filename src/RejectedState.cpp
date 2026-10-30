#include "RejectedState.h"
#include "Task.h"
#include "AvailableState.h"
#include "CancelledState.h"

void RejectedState :: cancel (Task* task){

    task -> handleChange (new CancelledState());

}

void RejectedState :: makeAvailable (Task* task){

    task -> setData ("rejectionReason", "");

    task -> handleChange (new AvailableState());

}

std::string RejectedState :: getName() const{

    return "Rejected";

}

TaskState* RejectedState :: clone() const{

    return new RejectedState(*this);

}

