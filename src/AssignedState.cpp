#include "AssignedState.h"
#include "Task.h"
#include "StartedState.h"
#include "RejectedState.h"
#include "CancelledState.h"

void AssignedState :: start (Task* task){

    task -> handleChange(new StartedState());

}

void AssignedState :: reject (Task* task, std::string reason){

    task -> setData ("rejectionReason", reason);

    task -> setAssignee (nullptr);

    task -> handleChange (new RejectedState());

}

void AssignedState :: cancel (Task* task) {

    task -> handleChange (new CancelledState());

}

std::string AssignedState :: getName() const{

    return "Assigned";

}

TaskState* AssignedState :: clone() const {

    return new AssignedState (*this);
    
}
