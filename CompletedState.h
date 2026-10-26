#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

class CompletedState : public TaskState{

    public:
        std::string getName() const override;

        TaskState* clone() const override;
        
};

#endif