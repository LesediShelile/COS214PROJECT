#include "../include/ParallelStage.h"

/**
 * @brief Constructs a ParallelStage.
 * @param itemID The stage's identifier.
 * @param name The stage's name.
 */
ParallelStage::ParallelStage(std::string itemID, std::string name) {
    (void)itemID;
    (void)name;
}

/**
 * @brief Copy-constructs a ParallelStage.
 * @param other The ParallelStage to copy.
 */
ParallelStage::ParallelStage(const ParallelStage& other) {
    (void)other;
}

/**
 * @brief Starts the stage.
 */
void ParallelStage::start() {
    activateChildren();
}

/**
 * @brief Cancels the stage.
 */
void ParallelStage::cancel() {
    activateChildren();
}

/**
 * @brief Completes the stage.
 */
void ParallelStage::complete() {
    activateChildren();
}

/**
 * @brief Makes the stage available.
 */
void ParallelStage::MakeAvailable() {
    activateChildren();
}

/**
 * @brief Checks whether the stage has finished.
 * @return True if the stage has finished.
 */
bool ParallelStage::isFinished() {
    return true;
}

/**
 * @brief Checks whether the stage is available.
 * @return True if the stage is available.
 */
bool ParallelStage::isAvailable() {
    return true;
}

/**
 * @brief Creates a copy of the stage.
 * @return A pointer to the cloned WorkItem.
 */
WorkItem* ParallelStage::clone() {
    return new ParallelStage(*this);
}

/**
 * @brief Activates the child work items.
 */
void ParallelStage::activateChildren() {
}

/**
 * @brief Destroys the ParallelStage.
 */
ParallelStage::~ParallelStage() {
}
