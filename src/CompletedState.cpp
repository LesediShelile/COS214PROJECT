#include "CompletedState.h"

std::string Completed :: getName() const{
    
    return "Completed";

}

TaskState* Completed :: clone() const{

    return new Completed (*this);

} 



