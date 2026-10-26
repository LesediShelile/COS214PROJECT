#include "CreatedState.h"
#include "Task.h"
#include "AvailableState.h"
#include "CancelledState.h"

void CreatedState :: makeAvailable (Task* task){

    task -> handleChange (new AvailableState());

}

std::string CreatedState :: getName() const{

    return "Created";

} 

TaskState * CreatedState :: clone() const {

    return new CreatedState (*this);

}

void CreatedState :: cancel (Task* task) {

    task -> handleChange (new CancelledState());

}


