#include "WorkItem.h"
#include "DepthFirstIterator.h"
#include "AvailableItemsIterator.h"

WorkItem::WorkItem(const std::string& itemID, const std::string& name)
    : itemID_(itemID), name_(name) {}

WorkItem::WorkItem(const WorkItem& other)
    : itemID_(other.itemID_), name_(other.name_), data_(other.data_) {}

WorkItem::~WorkItem() {}

std::string WorkItem::getItemID() const { return itemID_; }
std::string WorkItem::getName() const { return name_; }

void WorkItem::setData(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::string WorkItem::getData(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = data_.find(key);
    return it == data_.end() ? std::string() : it->second;
}

bool WorkItem::hasData(const std::string& key) const {
    return data_.find(key) != data_.end();
}

std::vector<WorkItem*> WorkItem::getChildren() const {
    return std::vector<WorkItem*>();
}

void WorkItem::activateChildren() {}

void WorkItem::setPublisher(EventPublisher*) {}

WorkItemIterator* WorkItem::createIterator() {
    return new DepthFirstIterator(this);
}

WorkItemIterator* WorkItem::createAvailableIterator() {
    return new AvailableItemsIterator(this);
}