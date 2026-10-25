#ifndef WORKITEMITERATOR_H
#define WORKITEMITERATOR_H

class WorkItem;

/**
 * @brief Iterator interface (Iterator pattern). Walks a WorkItem tree without
 *        exposing how a Stage stores its children.
 *
 * Usage:
 * @code
 * for (it->first(); !it->isDone(); it->next()) use(it->current());
 * @endcode
 * The iterator does not own the items it visits.
 */
class WorkItemIterator {
public:
    virtual ~WorkItemIterator() {}
    virtual void first() = 0;
    virtual void next() = 0;
    virtual bool isDone() const = 0;
    /** @return the current item, or NULL when isDone(). */
    virtual WorkItem* current() const = 0;
};

#endif
