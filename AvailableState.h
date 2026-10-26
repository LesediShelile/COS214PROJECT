#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

class AvailableState  : public TaskState {

    public: 

        void assign (Task* task, Participant* participant) override;

        void reject (Task* task, std::string reason) override;

        void cancel (Task* task) override;

        std::string getName() const override;

        TaskState* clone() const override;
};
#endif