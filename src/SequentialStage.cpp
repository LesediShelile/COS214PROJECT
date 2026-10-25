#include "../include/SequentialStage.h"

/**
 * @brief Constructs a SequentialStage.
 * @param itemID The stage's identifier.
 * @param name The stage's name.
 */
SequentialStage::SequentialStage(std::string itemID, std::string name)
    : Stage(itemID, name), currentChild(0) {
}

/**
 * @brief Copy-constructs a SequentialStage.
 * @param other The SequentialStage to copy.
 */
SequentialStage::SequentialStage(const SequentialStage& other)
    : Stage(other), currentChild(0) {
    for (unsigned int i = 0; i < other.children.size(); i++) {
        children.push_back(other.children[i]->clone());
    }

    if (!children.empty()) {
        currentChild = children[0];
    }
}

/**
 * @brief Starts the current child work item.
 */
void SequentialStage::start() {
    if (currentChild != 0) {
        currentChild->start();
    }
}

/**
 * @brief Cancels the current child work item.
 */
void SequentialStage::cancel() {
    if (currentChild != 0) {
        currentChild->cancel();
    }
}

/**
 * @brief Makes the stage available.
 */
void SequentialStage::makeAvailable() {
    if (!children.empty()) {
        currentChild = children[0];
        currentChild->makeAvailable();
    }
}

/**
 * @brief Checks whether all child work items have finished.
 * @return True if all children have finished.
 */
bool SequentialStage::isFinished() {
    if (children.empty()) {
        return true;
    }

    for (unsigned int i = 0; i < children.size(); i++) {
        if (!children[i]->isFinished()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Checks whether the stage is available.
 * @return True if the current child is available.
 */
bool SequentialStage::isAvailable() {
    return currentChild != 0 && currentChild->isAvailable();
}

/**
 * @brief Activates the stage's child work items sequentially.
 */
void SequentialStage::activateChildren() {
    for (unsigned int i = 0; i < children.size(); i++) {
        currentChild = children[i];
        currentChild->makeAvailable();
    }
}

/**
 * @brief Creates a copy of the stage.
 * @return A pointer to the cloned stage.
 */
WorkItem* SequentialStage::clone() {
    return new SequentialStage(*this);
}

/**
 * @brief Destroys the SequentialStage.
 */
SequentialStage::~SequentialStage() {
    for (unsigned int i = 0; i < children.size(); i++) {
        delete children[i];
    }

    children.clear();
    currentChild = 0;
}
