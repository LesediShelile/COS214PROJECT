#ifndef WORKITEM_H
#define WORKITEM_H

#include <map>
#include <string>
#include <vector>

class Participant;
class TaskMemento;
class WorkItemIterator;
class EventPublisher;

/**
 * @brief Component of the Composite pattern (also Component of Decorator,
 *        Aggregate of Iterator, Prototype, Receiver of Command and
 *        Originator of Memento).
 *
 * Every piece of work in a workflow, whether a single Task or a whole Stage,
 * is a WorkItem. The engine only ever talks to this interface, so it never
 * needs to know whether it holds a leaf or a group.
 *
 * Ownership: whoever holds a WorkItem* owns it, except where a method says
 * otherwise. A Stage owns its children and a TaskDecorator owns the item it
 * wraps.
 */
class WorkItem {
public:
    virtual ~WorkItem();

    /** @brief Unique id of this item inside its workflow. */
    virtual std::string getItemID() const;
    /** @brief Human readable name. */
    virtual std::string getName() const;

    /** @name Per-instance data (read by validation and rule based assignment) */
    ///@{
    virtual void setData(const std::string& key, const std::string& value);
    /** @return the stored value, or an empty string if the key is absent. */
    virtual std::string getData(const std::string& key) const;
    virtual bool hasData(const std::string& key) const;
    ///@}

    /** @name Lifecycle operations
     *  Invalid operations throw std::logic_error (see TaskState). */
    ///@{
    virtual void makeAvailable() = 0;
    virtual void assign(Participant* participant) = 0;
    virtual void start() = 0;
    virtual void complete() = 0;
    virtual void reject(const std::string& reason) = 0;
    virtual void cancel() = 0;
    ///@}

    /** @return true once the item can no longer change (completed/cancelled;
     *          for a Stage, when every child is finished). */
    virtual bool isFinished() const = 0;
    /** @return true if work can be picked up now. */
    virtual bool isAvailable() const = 0;
    /** @return name of the current lifecycle state, for display and history. */
    virtual std::string getStateName() const = 0;

    /**
     * @brief Prototype operation: deep copy of this item and everything it owns.
     * @return a new, independent item. The caller owns it.
     * Shared configuration (roles, strategies, publisher) is NOT copied, only
     * pointed to; work state, data and children ARE copied.
     */
    virtual WorkItem* clone() const = 0;

    /**
     * @brief Children of this item, or an empty list for a leaf.
     * Lets iterators walk any WorkItem without type checks or dynamic_cast.
     */
    virtual std::vector<WorkItem*> getChildren() const;

    /** @brief Re-evaluate which children should become available next.
     *  Default does nothing (a leaf has no children). Stages override it. */
    virtual void activateChildren();

    /** @brief Gives this item the publisher it reports events through
     *  (Observer). WorkflowInstance calls it on every item of a cloned tree.
     *  Default does nothing; Task stores it, TaskDecorator forwards it and
     *  AuditedTask also replaces its own. The publisher is shared, not owned. */
    virtual void setPublisher(EventPublisher* publisher);

    /** @brief Iterator over this item and all descendants, depth first.
     *  @return a new iterator, the caller deletes it. */
    virtual WorkItemIterator* createIterator();
    /** @brief Iterator over only the available leaf items.
     *  @return a new iterator, the caller deletes it. */
    virtual WorkItemIterator* createAvailableIterator();

    /** @brief Memento operations. Stages have no state of their own and
     *  return NULL / ignore restore. @return caller owns the memento. */
    virtual TaskMemento* createMemento() = 0;
    virtual void restoreMemento(TaskMemento* memento) = 0;

protected:
    WorkItem(const std::string& itemID, const std::string& name);
    /** Copy constructor used by clone() implementations. */
    WorkItem(const WorkItem& other);

    std::string itemID_;
    std::string name_;
    std::map<std::string, std::string> data_;

private:
    WorkItem& operator=(const WorkItem&); // not assignable
};

#endif