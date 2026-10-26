#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

class CreatedState : public TaskState {

    public:

        void makeAvailable (Task* task) override;

        std::string getName() const override;

        TaskState* clone() const override;

        void cancel (Task* task) override;
        
};
#endif