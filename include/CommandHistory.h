/**
 * @file CommandHistory.h
 * @brief Header file declaring the CommandHistory class for tracking command lifecycles.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>

class WorkflowCommand;

/**
 * @class CommandHistory
 * @brief Stack of executed commands used to support undo operations.
 * 
 * Manages a stack of executed WorkflowCommand objects and owns every command
 * pointer it contains. Copy operations are explicitly disabled to prevent multiple
 * ownership of the internal command pointers.
 * 
 * @see WorkflowCommand
 */
class CommandHistory {
public:
    /**
     * @brief Constructs an empty CommandHistory container.
     */
    CommandHistory();

    /**
     * @brief Destructor that cleans up and deallocates all contained commands.
     */
    ~CommandHistory();

    /**
     * @brief Deleted copy constructor to prevent duplicate ownership of commands.
     */
    CommandHistory(const CommandHistory&) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent duplicate ownership of commands.
     */
    CommandHistory& operator=(const CommandHistory&) = delete;

    /**
     * @brief Pushes a command onto the history stack and assumes ownership.
     * @param[in] command Pointer to the WorkflowCommand to store.
     * @throws std::invalid_argument If command is null.
     */
    void push(WorkflowCommand* command);

    /**
     * @brief Removes the most recent command and transfers ownership to the caller.
     * @return WorkflowCommand* Pointer to the popped command, or nullptr if history is empty.
     */
    WorkflowCommand* pop();

    /**
     * @brief Checks if the command history contains any commands.
     * @return true If the history stack is empty, false otherwise.
     */
    bool isEmpty() const;

    /**
     * @brief Returns the total number of commands stored in history.
     * @return int The current stack size.
     */
    int size() const;

private:
    std::vector<WorkflowCommand*> commands; ///< Vector acting as a stack of owned command pointers.
};

#endif