#ifndef WORKITEM_ITERATOR_H
#define WORKITEM_ITERATOR_H

#include <iostream>
using namespace std;

//forward declarations
class WorkItem;

class WorkItemIterator
{
    public:
        WorkItemIterator();
        virtual void first() = 0;
        virtual void next() = 0;
        virtual bool isDone() = 0;
        virtual WorkItem* current() = 0;
        virtual ~WorkItemIterator();
};

#endif //WORKITEM_ITERATOR_H