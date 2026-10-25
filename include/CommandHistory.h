#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

#include <vector>

class WorkflowCommand;

/**
 * Stack of executed commands. Owns every command it holds.
 */
class CommandHistory {
public:
    CommandHistory();
    ~CommandHistory();

    // Owns its commands, so copying is disabled.
    CommandHistory(const CommandHistory&) = delete;
    CommandHistory& operator=(const CommandHistory&) = delete;

    // Takes ownership of 'command'. Throws std::invalid_argument if null.
    void push(WorkflowCommand* command);

    // Removes the most recent command and gives ownership to the caller.
    // Returns nullptr if the history is empty.
    WorkflowCommand* pop();

    bool isEmpty() const;
    int size() const;

private:
    std::vector<WorkflowCommand*> commands;
};

#endif