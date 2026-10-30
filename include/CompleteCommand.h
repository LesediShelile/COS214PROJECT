/**
 * @file CompleteCommand.h
 * @brief Header file declaring the CompleteCommand concrete command class.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef COMPLETECOMMAND_H
#define COMPLETECOMMAND_H

#include <string>

#include "WorkflowCommand.h"

/**
 * @class CompleteCommand
 * @brief Concrete command that transitions a work item to its completed state.
 * 
 * Part of the Command design pattern. Encapsulates the completion of a WorkItem,
 * taking a snapshot before modifying state to enable rollback through undo operations.
 * 
 * @see WorkflowCommand
 * @see WorkItem
 */
class CompleteCommand : public WorkflowCommand {
public:
    /**
     * @brief Constructs a CompleteCommand targeting a specific work item.
     * @param[in] item Pointer to the WorkItem to be completed.
     */
    explicit CompleteCommand(WorkItem* item);

    /**
     * @brief Destructor for CompleteCommand.
     */
    ~CompleteCommand();

    /**
     * @brief Executes the completion command on the target work item.
     * @details Saves a backup memento before transitioning the item state.
     */
    void execute();

    /**
     * @brief Reverts the completion command.
     * @details Restores the item's previous state from the saved backup memento.
     */
    void undo();

    /**
     * @brief Returns a descriptive string summarizing the command action.
     * @return std::string Description of the completion command.
     */
    std::string getDescription() const;
};

#endif