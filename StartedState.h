#ifndef STARTSTATE_H
#define STARTSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"


class StartedState : public TaskState{

    public:

        void complete (Task* task);

        void reject (Task* task, std::string reason) override;

        void cancel (Task* task);

        std::string getName() const override;

        TaskState* clone() const override;
};

#endif