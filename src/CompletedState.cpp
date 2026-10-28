#include "CompletedState.h"

std::string CompletedState :: getName() const{
    
    return "Completed";

}

TaskState* CompletedState :: clone() const{

    return new CompletedState (*this);

} 



