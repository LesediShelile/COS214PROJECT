#include "../include/WorkItem.h"
#include "../include/WorkItemIterator.h"
#include "../include/TaskMemento.h"

/**
 * @brief Constructs a WorkItem with an ID and name.
 * @param itemID The work item's identifier.
 * @param name The work item's name.
 */
WorkItem::WorkItem(std::string itemID, std::string name) {
    this->itemID = itemID;
    this->name = name;
}

/**
 * @brief Returns the work item's identifier.
 * @return The work item's ID.
 */
std::string WorkItem::getItemID() {
    return itemID;
}

/**
 * @brief Returns the work item's name.
 * @return The work item's name.
 */
std::string WorkItem::getName() {
    return name;
}

/**
 * @brief Stores a value associated with a key.
 * @param key The data key.
 * @param value The value to store.
 */
void WorkItem::setData(std::string key, std::string value) {
    data[key] = value;
}

/**
 * @brief Retrieves the value associated with a key.
 * @param key The data key.
 * @return The stored value, or an empty string if the key does not exist.
 */
std::string WorkItem::getData(std::string key) {
    if (data.find(key) != data.end()) {
        return data[key];
    }

    return "";
}

/**
 * @brief Checks whether a data key exists.
 * @param key The data key.
 * @return True if the key exists, otherwise false.
 */
bool WorkItem::hasData(std::string key) {
    return data.find(key) != data.end();
}

/**
 * @brief Returns the name of the current state.
 * @return The current state name.
 */
std::string WorkItem::getStateName() {
    return "";
}

/**
 * @brief Virtual destructor for the abstract WorkItem class.
 */
WorkItem::~WorkItem() {
}
