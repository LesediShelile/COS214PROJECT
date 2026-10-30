#include "RejectedState.h"
#include "Task.h"
#include "AvailableState.h"
#include "CancelledState.h"

void RejectedState :: cancel (Task* task){

    task -> handleChange (new Cancelled());

}

void RejectedState :: makeAvailable (Task* task){

    task -> setData ("rejectionReason", "");

    task -> handleChange (new Available());

}

std::string RejectedState :: getName() const{

    return "Rejected";

}

TaskState* RejectedState :: clone() const{

    return new RejectedState(*this);

}

