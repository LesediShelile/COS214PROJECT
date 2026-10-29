/**
 * @file AssignCommand.h
 * @brief Header file declaring the AssignCommand concrete command class.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef ASSIGNCOMMAND_H
#define ASSIGNCOMMAND_H

#include <string>

#include "WorkflowCommand.h"

class Participant;

/**
 * @class AssignCommand
 * @brief Concrete command that assigns a work item to a participant.
 * 
 * Part of the Command design pattern. Stores a reference to the target Participant
 * and delegates assignment to the underlying WorkItem, while capturing snapshots
 * to allow undoing the assignment.
 * 
 * @see WorkflowCommand
 * @see WorkItem
 * @see Participant
 */
class AssignCommand : public WorkflowCommand {
public:
    /**
     * @brief Constructs an AssignCommand targeting a specific work item and participant.
     * @param[in] item Pointer to the WorkItem to be assigned.
     * @param[in] participant Pointer to the Participant receiving the assignment.
     */
    AssignCommand(WorkItem* item, Participant* participant);

    /**
     * @brief Destructor for AssignCommand.
     */
    ~AssignCommand();

    /**
     * @brief Executes the assignment command.
     * @details Backs up the item's current state and assigns it to the participant.
     */
    void execute();

    /**
     * @brief Reverts the assignment command.
     * @details Restores the item to its state prior to execution.
     */
    void undo();

    /**
     * @brief Returns a descriptive string summarizing the command action.
     * @return std::string Description of the assignment command.
     */
    std::string getDescription() const;

private:
    Participant* participant; ///< Non-owning pointer to the participant assigned to the work item.
};

#endif