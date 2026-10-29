/**
 * @file RejectCommand.h
 * @brief Header file declaring the RejectCommand concrete command class.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef REJECTCOMMAND_H
#define REJECTCOMMAND_H

#include <string>

#include "WorkflowCommand.h"

/**
 * @class RejectCommand
 * @brief Concrete command that transitions a work item to a rejected state with a reason.
 * 
 * Part of the Command design pattern. Encapsulates the rejection of a WorkItem,
 * saving a snapshot of the item's previous state to support rollback through undo operations.
 * 
 * @see WorkflowCommand
 * @see WorkItem
 */
class RejectCommand : public WorkflowCommand {
public:
    /**
     * @brief Constructs a RejectCommand targeting a specific work item with an explanation.
     * @param[in] item Pointer to the WorkItem to be rejected.
     * @param[in] reason Explanation describing why the work item was rejected.
     */
    RejectCommand(WorkItem* item, const std::string& reason);

    /**
     * @brief Destructor for RejectCommand.
     */
    ~RejectCommand();

    /**
     * @brief Executes the rejection command on the target work item.
     * @details Saves a backup memento prior to rejecting the item with the specified reason.
     */
    void execute();

    /**
     * @brief Reverts the rejection command.
     * @details Restores the item to its state prior to execution using the saved memento.
     */
    void undo();

    /**
     * @brief Returns a descriptive string summarizing the rejection action.
     * @return std::string Description of the rejection command including the reason.
     */
    std::string getDescription() const;

private:
    std::string reason; ///< Reason explanation associated with the work item rejection.
};

#endif