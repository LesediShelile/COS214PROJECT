#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

class RejectedState : public TaskState{

    public:

        void cancel (Task* task) override;

        void makeAvailable (Task* task) override;

        std::string getName() const override;

        TaskState* clone() const override;

};

#endif