#include "AvailableItemsIterator.h"
#include "WorkItem.h"

AvailableItemsIterator::AvailableItemsIterator(WorkItem* root) : inner_(root) {
    skipToAvailable();
}

AvailableItemsIterator::~AvailableItemsIterator() {}

void AvailableItemsIterator::first() {
    inner_.first();
    skipToAvailable();
}

void AvailableItemsIterator::next() {
    inner_.next();
    skipToAvailable();
}

bool AvailableItemsIterator::isDone() const { return inner_.isDone(); }

WorkItem* AvailableItemsIterator::current() const { return inner_.current(); }

void AvailableItemsIterator::skipToAvailable() {
    while (!inner_.isDone()) {
        WorkItem* item = inner_.current();
        if (item->isAvailable() && item->getChildren().empty()) return;
        inner_.next();
    }
}
