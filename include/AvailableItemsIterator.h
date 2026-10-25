#ifndef AVAILABLEITEMSITERATOR_H
#define AVAILABLEITEMSITERATOR_H

#include "WorkItemIterator.h"
#include "DepthFirstIterator.h"

/**
 * @brief Concrete iterator: yields only the available leaf items, i.e. the
 *        work a participant can pick up right now. Groups (items with
 *        children) are skipped.
 */
class AvailableItemsIterator : public WorkItemIterator {
public:
    explicit AvailableItemsIterator(WorkItem* root);
    virtual ~AvailableItemsIterator();

    virtual void first();
    virtual void next();
    virtual bool isDone() const;
    virtual WorkItem* current() const;

private:
    DepthFirstIterator inner_;
    /** Advance inner_ until it rests on an available leaf or is done. */
    void skipToAvailable();
};

#endif
