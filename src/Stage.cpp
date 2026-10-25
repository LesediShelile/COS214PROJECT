#include "Stage.h"
#include <stdexcept>

Stage::Stage(const std::string& itemID, const std::string& name)
    : WorkItem(itemID, name), activated_(false) {}

Stage::Stage(const Stage& other) : WorkItem(other), activated_(other.activated_) {
    try {
        for (std::size_t i = 0; i < other.children_.size(); ++i)
            children_.push_back(other.children_[i]->clone());
    } catch (...) {
        for (std::size_t i = 0; i < children_.size(); ++i) delete children_[i];
        throw;
    }
}

Stage::~Stage() {
    for (std::size_t i = 0; i < children_.size(); ++i) delete children_[i];
}

void Stage::add(WorkItem* item) {
    if (!item) throw std::invalid_argument("Stage::add: null item");
    children_.push_back(item);
}

std::vector<WorkItem*> Stage::getChildren() const { return children_; }

void Stage::makeAvailable() { activateChildren(); }

void Stage::assign(Participant*) {
    throw std::logic_error("assign() is not valid on a Stage; assign its Tasks");
}
void Stage::start() {
    throw std::logic_error("start() is not valid on a Stage; start its Tasks");
}
void Stage::complete() {
    throw std::logic_error("complete() is not valid on a Stage; complete its Tasks");
}
void Stage::reject(const std::string&) {
    throw std::logic_error("reject() is not valid on a Stage; reject its Tasks");
}

void Stage::cancel() {
    for (std::size_t i = 0; i < children_.size(); ++i)
        if (!children_[i]->isFinished()) children_[i]->cancel();
}

bool Stage::isFinished() const {
    for (std::size_t i = 0; i < children_.size(); ++i)
        if (!children_[i]->isFinished()) return false;
    return true;
}

bool Stage::isAvailable() const {
    for (std::size_t i = 0; i < children_.size(); ++i)
        if (children_[i]->isAvailable()) return true;
    return false;
}

std::string Stage::getStateName() const {
    if (isFinished()) return "Finished";
    return activated_ ? "InProgress" : "Pending";
}

TaskMemento* Stage::createMemento() { return 0; }
void Stage::restoreMemento(TaskMemento*) {}
