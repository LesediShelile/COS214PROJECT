/**
 * @file StartCommand.h
 * @brief Header file declaring the StartCommand concrete command class.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef STARTCOMMAND_H
#define STARTCOMMAND_H

#include <string>

#include "WorkflowCommand.h"

/**
 * @class StartCommand
 * @brief Command that starts a work item, transitioning it from Assigned to Started.
 * 
 * Part of the Command design pattern. Captures a memento snapshot of the target
 * WorkItem before starting execution so that undo() can return the item back
 * to its previous state.
 * 
 * @see WorkflowCommand
 * @see WorkItem
 */
class StartCommand : public WorkflowCommand {
public:
    /**
     * @brief Constructs a StartCommand targeting a specific work item.
     * @param[in] item Pointer to the WorkItem to be started.
     */
    explicit StartCommand(WorkItem* item);

    /**
     * @brief Destructor for StartCommand.
     */
    ~StartCommand();

    /**
     * @brief Executes the start command on the target work item.
     * @details Saves a backup memento before transitioning the item state.
     */
    void execute();

    /**
     * @brief Reverts the start command.
     * @details Restores the item to its previous state from the captured backup memento.
     */
    void undo();

    /**
     * @brief Returns a descriptive string summarizing the command action.
     * @return std::string Description of the start command.
     */
    std::string getDescription() const;
};

#endif