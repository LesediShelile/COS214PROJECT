#include "DepthFirstIterator.h"
#include "WorkItem.h"

DepthFirstIterator::DepthFirstIterator(WorkItem* root)
    : root_(root), currentItem_(0) {
    first();
}

DepthFirstIterator::~DepthFirstIterator() {}

void DepthFirstIterator::first() {
    while (!pending_.empty()) pending_.pop();
    currentItem_ = 0;
    if (root_) pending_.push(root_);
    advance();
}

void DepthFirstIterator::next() { advance(); }

bool DepthFirstIterator::isDone() const { return currentItem_ == 0; }

WorkItem* DepthFirstIterator::current() const { return currentItem_; }

void DepthFirstIterator::advance() {
    if (pending_.empty()) {
        currentItem_ = 0;
        return;
    }
    currentItem_ = pending_.top();
    pending_.pop();
    // push children in reverse so the first child is visited first
    std::vector<WorkItem*> kids = currentItem_->getChildren();
    for (std::vector<WorkItem*>::reverse_iterator it = kids.rbegin();
         it != kids.rend(); ++it) {
        pending_.push(*it);
    }
}
