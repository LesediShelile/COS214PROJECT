#include "ParallelStage.h"

ParallelStage::ParallelStage(const std::string& itemID, const std::string& name)
    : Stage(itemID, name) {}

ParallelStage::ParallelStage(const ParallelStage& other) : Stage(other) {}

ParallelStage::~ParallelStage() {}

void ParallelStage::activateChildren() {
    const bool first = !activated_;
    activated_ = true;
    for (std::size_t i = 0; i < children_.size(); ++i) {
        if (children_[i]->isFinished()) continue;
        if (first) children_[i]->makeAvailable();
        else       children_[i]->activateChildren();
    }
}

WorkItem* ParallelStage::clone() const { return new ParallelStage(*this); }
