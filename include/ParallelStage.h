#ifndef PARALLELSTAGE_H
#define PARALLELSTAGE_H
#include <string>
#include "WorkItem.h"

/**
 * @brief Represents a stage that processes work items in parallel.
 */
class ParallelStage
{
public:
    /**
     * @brief Constructs a ParallelStage.
     * @param itemID The stage's identifier.
     * @param name The stage's name.
     */
    ParallelStage(std::string* itemID, std::string* name);

    /**
     * @brief Copy-constructs a ParallelStage.
     * @param other The ParallelStage to copy.
     */
    ParallelStage(const ParallelStage& other);

    /** @brief Starts the stage. */
    void start();

    /** @brief Cancels the stage. */
    void cancel();

    /** @brief Completes the stage. */
    void complete();

    /** @brief Makes the stage available. */
    void MakeAvailable();

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

    /**
     * @brief Creates a copy of the stage.
     * @return A pointer to the cloned WorkItem.
     */
    WorkItem* clone();

    /** @brief Activates the child work items. */
    void activateChildren();

    /** @brief Destroys the ParallelStage. */
    ~ParallelStage();
};

#endif // PARALLELSTAGE_H
