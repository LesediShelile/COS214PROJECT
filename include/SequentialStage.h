#ifndef SEQUENTIALSTAGE_H
#define SEQUENTIALSTAGE_H
#include "Stage.h"
#include "WorkItem.h"

/**
 * @brief Represents a stage that processes its children sequentially.
 *
 * SequentialStage maintains a pointer to the current child work item.
 */
class SequentialStage : public Stage
{
private:
    WorkItem* currentChild;

public:
    /**
     * @brief Constructs a SequentialStage.
     * @param itemID The stage's identifier.
     * @param name The stage's name.
     */
    SequentialStage(std::string* itemID, std::string* name);

    /**
     * @brief Copy-constructs a SequentialStage.
     * @param other The SequentialStage to copy.
     */
    SequentialStage(const SequentialStage& other);

    /** @brief Starts the current stage. */
    void start();

    /** @brief Cancels the current stage. */
    void cancel();

    /** @brief Makes the stage available. */
    void makeAvailable();

    /**
     * @brief Checks whether the stage has finished.
     * @return True if the stage has finished.
     */
    bool isFinished();

    /**
     * @brief Checks whether the stage is available.
     * @return True if the stage is available.
     */
    bool isAvailable();

    /** @brief Activates the stage's child work items. */
    void activateChildren();

    /**
     * @brief Creates a copy of the stage.
     * @return A pointer to the cloned WorkItem.
     */
    WorkItem* clone();

    /** @brief Destroys the SequentialStage. */
    ~SequentialStage();
};

#endif // SEQUENTIALSTAGE_H