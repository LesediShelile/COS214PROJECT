#include "AvailableState.h"
#include "Task.h"
#include "AssignedState.h"
#include "RejectedState.h"
#include "CancelledState.h"
#include <iostream>

void AvailableState :: assign (Task* task, Participant* participant){

    if (participant == nullptr){
        std::cout << "Cannot assign: no participant given" << std::endl;

        return;
    }

    task -> setAssignee (participant);

    task -> handleChange (new AssignedState());

}

void AvailableState :: reject (Task* task, const std::string reason) {

    task -> setData ("rejectionReason", reason);

    task -> handleChange (new RejectedState());

}

void AvailableState :: cancel (Task* task) {

    task -> handleChange (new CancelledState());

}

std::string AvailableState :: getName() const {

    return "Available";

}

TaskState* AvailableState :: clone() const{

    return new AvailableState (*this);

}


