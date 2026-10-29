/**
 * @file CommandHistory.cpp
 * @brief Implementation of the CommandHistory class for managing executed workflow commands.
 * @author Kiara Ajodhaparsadh
 * @date 2026-10-29
 */

#include "../include/CommandHistory.h"

#include <stdexcept>

#include "../include/WorkflowCommand.h"

/**
 * @brief Default constructor for CommandHistory.
 */
CommandHistory::CommandHistory() {}

/**
 * @brief Destructor that deallocates all remaining stored commands.
 */
CommandHistory::~CommandHistory() {
    for (auto& command : commands) {
        delete command;
    }
    commands.clear();
}

/**
 * @brief Pushes an executed command onto the history stack.
 * @param[in] command Pointer to the WorkflowCommand to store.
 * @throws std::invalid_argument If the command pointer is null.
 */
void CommandHistory::push(WorkflowCommand* command) {
    if (command == nullptr) {
        throw std::invalid_argument("CommandHistory::push: command is null");
    }
    commands.push_back(command);
}

/**
 * @brief Pops and returns the most recently pushed command from the history stack.
 * @return WorkflowCommand* Pointer to the popped command, or nullptr if the history is empty.
 */
WorkflowCommand* CommandHistory::pop() {
    if (commands.empty()) {
        return nullptr;
    }
    WorkflowCommand* command = commands.back();
    commands.pop_back();
    return command;
}

/**
 * @brief Checks whether the command history stack is empty.
 * @return true If there are no commands in history, false otherwise.
 */
bool CommandHistory::isEmpty() const {
    return commands.empty();
}

/**
 * @brief Returns the total number of commands currently stored in history.
 * @return int The number of stored commands.
 */
int CommandHistory::size() const {
    return static_cast<int>(commands.size());
}