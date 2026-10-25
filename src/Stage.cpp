#include "../include/Stage.h"
#include "../include/TaskMemento.h"

/**
 * @brief Constructs a Stage with an ID and name.
 * @param itemID The stage's identifier.
 * @param name The stage's name.
 */
Stage::Stage(std::string itemID, std::string name)
    : children() {
    (void)itemID;
    (void)name;
}

/**
 * @brief Assigns a participant to each child work item.
 * @param participant The participant to assign.
 */
void Stage::assign(Participant* participant) {
    for (unsigned int i = 0; i < children.size(); i++) {
        children[i]->assign(participant);
    }
}

/**
 * @brief Rejects the stage's child work items.
 * @param reason The reason for rejection.
 */
void Stage::reject(std::string* reason) {
    for (unsigned int i = 0; i < children.size(); i++) {
        children[i]->reject(*reason);
    }
}

/**
 * @brief Creates a copy of the stage.
 * @return A pointer to the cloned stage.
 */
WorkItem* Stage::clone() {
    return 0;
}

/**
 * @brief Creates an iterator for the stage.
 * @return A pointer to a WorkItemIterator.
 */
WorkItemIterator* Stage::createIterator() {
    return 0;
}

/**
 * @brief Creates a memento of the stage.
 * @return A pointer to a TaskMemento.
 */
TaskMemento* Stage::createMemento() {
    return 0;
}

/**
 * @brief Restores the stage from a memento.
 * @param memento The memento to restore.
 */
void Stage::restoreMemento(TaskMemento* memento) {
    (void)memento;
}

/**
 * @brief Adds a child work item to the stage.
 * @param item The work item to add.
 */
void Stage::add(WorkItem* item) {
    if (item != 0) {
        children.push_back(item);
    }
}

/**
 * @brief Returns the stage's child work items.
 * @return A vector containing the child work items.
 */
std::vector<WorkItem*> Stage::getChildren() {
    return children;
}

/**
 * @brief Activates the child work items.
 */
void Stage::activateChildren() {
    for (unsigned int i = 0; i < children.size(); i++) {
        children[i]->makeAvailable();
    }
}

/**
 * @brief Returns the stage's current state name.
 * @return The state name.
 */
std::string Stage::getStateName() {
    return "";
}

/**
 * @brief Creates an iterator over available work items.
 * @return A pointer to a WorkItemIterator.
 */
WorkItemIterator* Stage::createAvailableIterator() {
    return 0;
}

/**
 * @brief Destroys the Stage.
 */
Stage::~Stage() {
}
