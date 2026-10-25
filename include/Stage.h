#ifndef STAGE_H
#define STAGE_H

#include <vector>
#include <string>
#include "WorkItem.h"
#include "Participant.h"
#include "WorkItemIterator.h"

/**
 * @brief Represents an abstract stage containing child work items.
 *
 * Stage stores WorkItem objects and provides operations for working
 * with its children. Derived stages implement the task lifecycle methods.
 */
class Stage
{
protected:
    std::vector<WorkItem*> children;

    /**
     * @brief Constructs a Stage.
     * @param itemID The stage's identifier.
     * @param name The stage's name.
     */
    Stage(std::string* itemID, std::string* name);

public:
    /**
     * @brief Assigns a participant to the stage.
     * @param participant The participant to assign.
     */
    void assign(Participant* participant);

    /**
     * @brief Rejects the stage.
     * @param reason The reason for rejection.
     */
    void reject(std::string* reason);

    /**
     * @brief Creates a copy of the stage.
     * @return A pointer to the cloned WorkItem.
     */
    WorkItem* clone();

    /**
     * @brief Creates an iterator for the stage.
     * @return A pointer to a WorkItemIterator.
     */
    WorkItemIterator* createIterator();

    /**
     * @brief Creates a memento of the stage.
     * @return A pointer to a TaskMemento.
     */
    TaskMemento* createMemento();

    /**
     * @brief Restores the stage from a memento.
     * @param memento The memento to restore.
     */
    void restoreMemento(TaskMemento* memento);

    /** @brief Starts the stage. */
    virtual void start() = 0;

    /** @brief Cancels the stage. */
    virtual void cancel() = 0;

    /** @brief Completes the stage. */
    virtual void complete() = 0;

    /** @brief Makes the stage available. */
    virtual void makeAvailable() = 0;

    /**
     * @brief Checks whether the stage has finished.
     * @return True if the stage has finished.
     */
    virtual bool isFinished() = 0;

    /**
     * @brief Checks whether the stage is available.
     * @return True if the stage is available.
     */
    virtual bool isAvailable() = 0;

    /**
     * @brief Adds a child work item to the stage.
     * @param item The work item to add.
     */
    void add(WorkItem* item);

    /**
     * @brief Gets the stage's child work items.
     * @return A vector containing the child work items.
     */
    std::vector<WorkItem*> getChildren();

    /** @brief Activates the child work items. */
    void activateChildren();

    /**
     * @brief Gets the stage's current state name.
     * @return The state name.
     */
    std::string getStateName();

    /**
     * @brief Creates an iterator over available work items.
     * @return A pointer to a WorkItemIterator.
     */
    WorkItemIterator* createAvailableIterator();

    /** @brief Destroys the Stage. */
    virtual ~Stage() = 0;
};

#endif // STAGE_H