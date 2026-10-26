#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <iostream>
#include <string>
#include <vector>

class Task;
class Participant;

class AssignmentStrategy {

    public:

        virtual ~AssignmentStrategy();

        virtual Participant* selectParticipant (Task* task, std::vector<Participant*> participants) = 0;

};

#endif

