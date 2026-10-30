#include "AssignedState.h"
#include "Task.h"
#include "StartedState.h"
#include "RejectedState.h"
#include "CancelledState.h"

void Assigned :: start (Task* task){

    task -> handleChange(new StartedState());

}

void Assigned :: reject (Task* task, std::string reason){

    task -> setData ("rejectionReason", reason);

    task -> setAssignee (nullptr);

    task -> handleChange (new RejectedState());

}

void Assigned :: cancel (Task* task) {

    task -> handleChange (new Cancelled());

}

std::string Assigned :: getName() const{

    return "Assigned";

}

TaskState* Assigned :: clone() const {

    return new Assigned (*this);
    
}
