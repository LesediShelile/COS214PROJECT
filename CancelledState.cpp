#include "CancelledState.h"

std::string CancelledState :: getName() const {

    return "Cancelled";

}

TaskState* CancelledState :: clone() const {

    return new CancelledState (*this);

}


