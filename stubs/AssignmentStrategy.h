#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H
#include <vector>
class Task;
class Participant;
/** STUB: replace with Cameron's real AssignmentStrategy. */
class AssignmentStrategy {
public:
    virtual ~AssignmentStrategy() {}
    virtual Participant* selectParticipant(Task* task,
                                           const std::vector<Participant*>& participants) = 0;
};
#endif
