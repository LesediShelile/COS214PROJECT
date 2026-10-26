#ifndef TASKSTATE_H
#define TASKSTATE_H

#include <iostream>
#include <string>

class Task;
class Participant;

//Abstract Class

class TaskState {

    public: 

    virtual ~TaskState(); //Destructor

    virtual void assign(Task* task, Participant* participants);

    virtual void start (Task* task);

    virtual void complete (Task* task); 

    virtual void reject(Task* task, std::string reason);

    virtual void cancel (Task* task);

    virtual void makeAvailable (Task* task);

    virtual std::string getName() const = 0; //Pure virtual 

    virtual TaskState* clone() const = 0; //Pure virtual 



};

#endif

  