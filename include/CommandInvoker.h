#ifndef COMMANDINVOKER_H
#define COMMANDINVOKER_H

#include "CommandHistory.h"

class WorkflowCommand;

/**
 * Invoker: runs commands and keeps the ones that succeeded so they can be undone.
 */
class CommandInvoker {
public:
    CommandInvoker();
    ~CommandInvoker();

    // Takes ownership of 'command'. If execute() throws (for example an invalid
    // state transition), the command is deleted and is not added to the history.
    void executeCommand(WorkflowCommand* command);

    // Undoes the most recent command. Returns false if there is nothing to undo.
    // The undone command is discarded (there is no redo). If undo() throws,
    // the command is still discarded and the exception propagates.
    bool undoLast();

    bool canUndo() const;
    CommandHistory* getHistory();

private:
    CommandHistory history;
};

#endif