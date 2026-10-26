#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

class CancelledState : public TaskState{

    public:

        std::string getName() const override;

        TaskState* clone() const override;
};

#endif