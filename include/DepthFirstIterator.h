#ifndef DEPTH_FIRST_ITERATOR_H
#define DEPTH_FIRST_ITERATOR_H

#include <iostream>
#include <stack>
using namespace std;

#include "WorkItemIterator.h"
#include "Stub.h" // <--- delete and replace

class DepthFirstIterator : public WorkItemIterator
{
    public:
        DepthFirstIterator(WorkItem* root);
        void first() override;
        void next() override;
        bool isDone() override;
        WorkItem* current() override;
        virtual ~DepthFirstIterator();
    private:
        WorkItem* root;
        stack<WorkItem*> pending;
        WorkItem* currentItem;
};

#endif //DEPTH_FIRST_ITERATOR_H