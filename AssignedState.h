#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include <iostream>
#include <string>

#include "TaskState.h"

class AssignedState  : public TaskState {

    public:

        void start (Task* start) override;

        void reject(Task* task, std::string reason) override;

        void cancel (Task* task)  override;

        std::string getName() const override;

        TaskState* clone() const override;
};
#endif