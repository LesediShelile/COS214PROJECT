#ifndef TASKDECORATOR_H
#define TASKDECORATOR_H

#include "WorkItem.h"

/**
 * @brief Decorator base: wraps another WorkItem and forwards every call to it.
 *        Concrete decorators (ValidatedTask, AuditedTask, PriorityTask)
 *        override only what they extend.
 *
 * The decorator owns the item it wraps. Its copy constructor clones the
 * wrapped item, so a concrete decorator's clone() is simply
 * `return new MyDecorator(*this);`.
 */
class TaskDecorator : public WorkItem {
public:
    virtual ~TaskDecorator();

    virtual std::string getItemID() const;
    virtual std::string getName() const;
    virtual void setData(const std::string& key, const std::string& value);
    virtual std::string getData(const std::string& key) const;
    virtual bool hasData(const std::string& key) const;

    virtual void makeAvailable();
    virtual void assign(Participant* participant);
    virtual void start();
    virtual void complete();
    virtual void reject(const std::string& reason);
    virtual void cancel();

    virtual bool isFinished() const;
    virtual bool isAvailable() const;
    virtual std::string getStateName() const;

    virtual std::vector<WorkItem*> getChildren() const;
    virtual void activateChildren();

    /** @brief Forwards the publisher to the wrapped item. */
    virtual void setPublisher(EventPublisher* publisher);

    virtual TaskMemento* createMemento();
    virtual void restoreMemento(TaskMemento* memento);

    /** @brief Subclasses return a deep copy of themselves (see class note). */
    virtual WorkItem* clone() const = 0;

    /** @return the wrapped item (not a copy). */
    WorkItem* getWrapped() const;

protected:
    /** @throws std::invalid_argument if @p task is NULL. Takes ownership. */
    explicit TaskDecorator(WorkItem* task);
    TaskDecorator(const TaskDecorator& other);

private:
    WorkItem* task_;
    TaskDecorator& operator=(const TaskDecorator&);
    static WorkItem* require(WorkItem* task);
};

#endif