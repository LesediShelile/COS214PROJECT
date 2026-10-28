#ifndef AVAILABLE_ITEMS_ITERATOR_H
#define AVAILABLE_ITEMS_ITERATOR_H

#include <iostream>
using namespace std;

#include "WorkItemIterator.h"
#include "DepthFirstIterator.h"

class AvailableItemsIterator : public WorkItemIterator
{
    public:
        AvailableItemsIterator(WorkItem* root);
        void first() override;
        void next() override;
        bool isDone() override;
        WorkItem* current() override;
        void skipToAvailable();
        virtual ~AvailableItemsIterator(){};
    private:
        DepthFirstIterator* inner;
};

#endif //AVAILABLE_ITEMS_ITERATOR_H