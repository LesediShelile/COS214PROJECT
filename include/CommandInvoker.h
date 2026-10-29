/**
 * @file CommandInvoker.h
 * @brief Header file declaring the CommandInvoker class for executing and managing command undo logic.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include "CommandHistory.h"

class WorkflowCommand;

/**
 * @class CommandInvoker
 * @brief Invoker in the Command pattern responsible for executing commands and managing rollback history.
 * 
 * Runs workflow commands and maintains successful executions within an internal
 * CommandHistory instance to support single-level or multi-level undo operations.
 * 
 * @see CommandHistory
 * @see WorkflowCommand
 */
class CommandInvoker {
public:
    /**
     * @brief Constructs a new CommandInvoker.
     */
    CommandInvoker();

    /**
     * @brief Destructor for CommandInvoker.
     */
    ~CommandInvoker();

    /**
     * @brief Executes a command and stores it in history upon success.
     * @details Takes ownership of the passed command. If execution throws an exception,
     * the command is safely deleted and excluded from history.
     * @param[in] command Pointer to the WorkflowCommand to run.
     */
    void executeCommand(WorkflowCommand* command);

    /**
     * @brief Undoes the most recently executed command in the history.
     * @details Discards the command after undoing (no redo functionality). If the
     * undo action throws an exception, the command is discarded and the exception propagates.
     * @return true If a command was successfully undone, false if the history is empty.
     */
    bool undoLast();

    /**
     * @brief Checks if there are any commands currently available to undo.
     * @return true If undo operations can be performed, false otherwise.
     */
    bool canUndo() const;

    /**
     * @brief Retrieves a pointer to the internal history stack.
     * @return CommandHistory* Pointer to the internal CommandHistory instance.
     */
    CommandHistory* getHistory();

private:
    CommandHistory history; ///< Stack maintaining executed commands that are eligible for undo.
};

#endif