#include "SequentialStage.h"

SequentialStage::SequentialStage(const std::string& itemID, const std::string& name)
    : Stage(itemID, name), currentChild_(0), currentActivated_(false) {}

SequentialStage::SequentialStage(const SequentialStage& other)
    : Stage(other), currentChild_(other.currentChild_),
      currentActivated_(other.currentActivated_) {}

SequentialStage::~SequentialStage() {}

void SequentialStage::activateChildren() {
    activated_ = true;
    // step over children that finished since the last call
    while (currentChild_ < children_.size() && children_[currentChild_]->isFinished()) {
        ++currentChild_;
        currentActivated_ = false;
    }
    if (currentChild_ >= children_.size()) return;

    WorkItem* child = children_[currentChild_];
    if (!currentActivated_) {
        child->makeAvailable();
        currentActivated_ = true;
    } else {
        child->activateChildren();   // lets a nested stage progress itself
    }
}

WorkItem* SequentialStage::clone() const { return new SequentialStage(*this); }
