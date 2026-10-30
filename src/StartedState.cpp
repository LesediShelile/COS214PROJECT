#include "StartedState.h"
#include "Task.h"
#include "CompletedState.h"
#include "RejectedState.h"
#include "CancelledState.h"

void StartedState :: complete (Task* task){ //Makes a new complete state object 
    
    task -> handleChange(new Completed()); 

}

void StartedState :: reject (Task* task, std::string reason){ //Makes a new rejected state object, when the work isnt acceptable

    task -> setData("rejectionReason", reason); 

    task -> setAssignee(nullptr); //removes who was assigned to this task to be given to someone else later

    task -> handleChange(new RejectedState());

}

void StartedState :: cancel (Task* task) { //moves the task to cancelled(end state)

    task -> handleChange(new Cancelled());
}

std::string StartedState :: getName() const{

    return "Started";

}
TaskState* StartedState :: clone() const{

    return new StartedState(*this);

}

