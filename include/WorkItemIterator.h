#ifndef WORKITEM_ITERATOR_H
#define WORKITEM_ITERATOR_H

#include <iostream>

//forward declarations
class WorkItem;

class WorkItemIterator
{
    public:
        WorkItemIterator() = default;
        virtual void first() = 0;
        virtual void next() = 0;
        virtual bool isDone() = 0;
        virtual WorkItem* current() = 0;
        virtual ~WorkItemIterator() = default;
};

#endif //WORKITEM_ITERATOR_H