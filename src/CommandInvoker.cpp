/**
 * @file CommandInvoker.cpp
 * @brief Implementation of the CommandInvoker class executing and undoing workflow commands.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#include "../include/CommandInvoker.h"

#include <memory>
#include <stdexcept>

#include "../include/WorkflowCommand.h"

/**
 * @brief Default constructor for CommandInvoker.
 */
CommandInvoker::CommandInvoker() {}

/**
 * @brief Destructor for CommandInvoker.
 */
CommandInvoker::~CommandInvoker() {}

/**
 * @brief Executes a given workflow command and tracks it in history for undo capability.
 * @details Employs std::unique_ptr during execution to ensure exception-safe cleanup.
 * If execution completes successfully, ownership transfers to the internal CommandHistory.
 * @param[in] command Pointer to the WorkflowCommand to execute.
 * @throws std::invalid_argument If the command pointer is null.
 */
void CommandInvoker::executeCommand(WorkflowCommand* command) {
    if (command == nullptr) {
        throw std::invalid_argument("executeCommand: command is null");
    }
    std::unique_ptr<WorkflowCommand> owned(command);
    owned->execute();            // if this throws, 'owned' deletes the command
    history.push(owned.get());   // history now owns it...
    owned.release();             // ...so stop owning it here - only 1 owner
}

/**
 * @brief Pops and undoes the most recently executed workflow command.
 * @return true If a command was successfully undone, false if no commands exist in history.
 */
bool CommandInvoker::undoLast() {
    if (history.isEmpty()) {
        return false;
    }
    std::unique_ptr<WorkflowCommand> command(history.pop());
    command->undo();
    return true;
}

/**
 * @brief Checks if there are any commands available in the history to undo.
 * @return true If undo is possible, false otherwise.
 */
bool CommandInvoker::canUndo() const {
    return !history.isEmpty();
}

/**
 * @brief Retrieves a pointer to the internal command history tracker.
 * @return CommandHistory* Pointer to the internal CommandHistory instance.
 */
CommandHistory* CommandInvoker::getHistory() {
    return &history;
}