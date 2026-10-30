#include "CreatedState.h"
#include "Task.h"
#include "AvailableState.h"
#include "CancelledState.h"

void Created :: makeAvailable (Task* task){

    task -> handleChange (new Available());

}

std::string Created :: getName() const{

    return "Created";

} 

TaskState * Created :: clone() const {

    return new Created (*this);

}

void Created :: cancel (Task* task) {

    task -> handleChange (new Cancelled());

}


