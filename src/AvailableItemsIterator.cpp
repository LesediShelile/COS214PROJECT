#include "AvailableItemsIterator.h"
#include "WorkItem.h"

AvailableItemsIterator::AvailableItemsIterator(WorkItem* root)
{
    this->inner = new DepthFirstIterator(root);
}

void AvailableItemsIterator::skipToAvailable()
{
    while(!inner->isDone() && !inner->current()->isAvailable())
    {
        inner->next();
    }
}

void AvailableItemsIterator::first()
{
    this->inner->first();
    skipToAvailable();
}

void AvailableItemsIterator::next()
{
    this->inner->next();
    skipToAvailable();
}

bool AvailableItemsIterator::isDone()
{
    return this->inner->isDone();
}

WorkItem* AvailableItemsIterator::current()
{
    return this->inner->current();
}