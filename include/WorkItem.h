#ifndef WORKITEM_H
#define WORKITEM_H

#include <map>
#include <string>
#include "Participant.h"

// Forward declarations for classes used by pointers.
class WorkItemIterator;
class TaskMemento;

/**
 * @brief Abstract base class for all workflow items.
 *
 * WorkItem is the common component in the Composite pattern.
 * It provides shared data and operations for individual tasks
 * and composite stages.
 */
class WorkItem {

protected:
    /** @brief Unique identifier of the work item. */
    std::string itemID;

    /** @brief Name of the work item. */
    std::string name;

    /** @brief Data associated with the work item. */
    std::map<std::string, std::string> data;

protected:
    /**
     * @brief Constructs a WorkItem.
     * @param itemID Identifier of the work item.
     * @param name Name of the work item.
     */
    WorkItem(std::string itemID, std::string name);

    /** @brief Returns the work item's identifier. */
    std::string getItemID();

    /** @brief Returns the work item's name. */
    std::string getName();

    /**
     * @brief Stores a value associated with a key.
     * @param key Data key.
     * @param value Value to store.
     */
    void setData(std::string key, std::string value);

    /**
     * @brief Retrieves the value associated with a key.
     * @param key Data key.
     * @return Stored value.
     */
    std::string getData(std::string key);

    /**
     * @brief Checks whether a data key exists.
     * @param key Data key.
     * @return True if the key exists, otherwise false.
     */
    bool hasData(std::string key);

    /** @brief Makes the work item available. */
    virtual void makeAvailable() = 0;

    /**
     * @brief Assigns a participant to the work item.
     * @param participant Participant being assigned.
     */
    virtual void assign(Participant* participant) = 0;

    /** @brief Starts the work item. */
    virtual void start() = 0;

    /** @brief Marks the work item as completed. */
    virtual void complete() = 0;

    /**
     * @brief Rejects the work item.
     * @param reason Reason for rejection.
     */
    virtual void reject(std::string reason) = 0;

    /** @brief Cancels the work item. */
    virtual void cancel() = 0;

    /**
     * @brief Checks whether the work item has finished.
     * @return True if finished, otherwise false.
     */
    virtual bool isFinished() = 0;

    /**
     * @brief Checks whether the work item is available.
     * @return True if available, otherwise false.
     */
    virtual bool isAvailable() = 0;

    /**
     * @brief Creates a copy of the work item.
     * @return Pointer to the cloned work item.
     */
    virtual WorkItem* clone() = 0;

    /**
     * @brief Creates an iterator for the work item's children.
     * @return Pointer to the created iterator.
     */
    virtual WorkItemIterator* createIterator() = 0;

    /**
     * @brief Creates a snapshot of the work item's state.
     * @return Pointer to the created memento.
     */
    virtual TaskMemento* createMemento() = 0;

    /**
     * @brief Restores the work item's state from a memento.
     * @param memento Memento containing the saved state.
     */
    virtual void restoreMemento(TaskMemento* memento) = 0;

    /**
     * @brief Returns the name of the current state.
     * @return State name.
     */
    std::string getStateName();

    /**
     * @brief Creates an iterator over available work items.
     * @return Pointer to the created iterator.
     */
    virtual WorkItemIterator* createAvailableIterator() = 0;

    /** @brief Virtual destructor for the abstract base class. */
    virtual ~WorkItem() = 0;
};

#endif // WORKITEM_H
