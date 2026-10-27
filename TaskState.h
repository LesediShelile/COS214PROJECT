#ifndef TASKSTATE_H
#define TASKSTATE_H

#include <iostream>
#include <string>

class Task;
class Participant;

//Abstract Class

/**
 * @brief Abstract state of a Task (State pattern).
 *
 * Each concrete state overrides only the actions it allows. Actions that
 * are not overridden report that they cannot be done, and the task stays
 * in its current state.
 */
class TaskState {

    public: 

    /** @brief Virtual destructor so concrete states are cleaned up correctly. */
    virtual ~TaskState(); //Destructor

    /**
     * @brief Assigns a participant to the task.
     * @param task The task this state belongs to.
     * @param participants The participant to assign.
     */
    virtual void assign(Task* task, Participant* participants);

    /**
     * @brief Starts work on the task.
     * @param task The task this state belongs to.
     */
    virtual void start (Task* task);

    /**
     * @brief Completes the task.
     * @param task The task this state belongs to.
     */
    virtual void complete (Task* task); 

    /**
     * @brief Rejects the task.
     * @param task The task this state belongs to.
     * @param reason Why the task was rejected.
     */
    virtual void reject(Task* task, std::string reason);

    /**
     * @brief Cancels the task.
     * @param task The task this state belongs to.
     */
    virtual void cancel (Task* task);

    /**
     * @brief Makes the task ready to be assigned.
     * @param task The task this state belongs to.
     */
    virtual void makeAvailable (Task* task);

    /**
     * @brief Gets the name of this state.
     * @return The state's name, e.g. "Started".
     */
    virtual std::string getName() const = 0; //Pure virtual 

    /**
     * @brief Creates a copy of this state (Prototype pattern).
     * @return A new state object of the same type.
     */
    virtual TaskState* clone() const = 0; //Pure virtual 



};

#endif