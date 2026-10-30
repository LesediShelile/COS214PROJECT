#include "CancelledState.h"

std::string Cancelled :: getName() const {

    return "Cancelled";

}

TaskState* Cancelled :: clone() const {

    return new Cancelled (*this);

}



