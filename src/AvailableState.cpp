#include "AvailableState.h"
#include "Task.h"
#include "AssignedState.h"
#include "RejectedState.h"
#include "CancelledState.h"
#include <iostream>

void Available :: assign (Task* task, Participant* participant){

    if (participant == nullptr){
        std::cout << "Cannot assign: no participant given" << std::endl;

        return;
    }

    task -> setAssignee (participant);

    task -> handleChange (new Assigned());

}

void Available :: reject (Task* task, const std::string reason) {

    task -> setData ("rejectionReason", reason);

    task -> handleChange (new RejectedState());

}

void Available :: cancel (Task* task) {

    task -> handleChange (new Cancelled());

}

std::string Available :: getName() const {

    return "Available";

}

TaskState* Available :: clone() const{

    return new Available (*this);

}



