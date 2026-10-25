#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include <stack>
#include "WorkItemIterator.h"

/**
 * @brief Concrete iterator: pre-order depth-first walk (parent before its
 *        children, children in the order they were added).
 */
class DepthFirstIterator : public WorkItemIterator {
public:
    explicit DepthFirstIterator(WorkItem* root);
    virtual ~DepthFirstIterator();

    virtual void first();
    virtual void next();
    virtual bool isDone() const;
    virtual WorkItem* current() const;

private:
    WorkItem* root_;
    std::stack<WorkItem*> pending_;
    WorkItem* currentItem_;
    void advance();
};

#endif
