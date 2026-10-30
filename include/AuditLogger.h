
#ifndef AUDITLOGGER_H
#define AUDITLOGGER_H

#include <vector>
#include <string>
#include "WorkflowListener.h"

/**
 * @brief Records details from workflow events.
 */
class AuditLogger : public WorkflowListener {
private:
    /** @brief Recorded event messages. */
    std::vector<std::string> sentMessages;

public:
    /** @brief Constructs an AuditLogger. */
    AuditLogger();

    /**
     * @brief Records details from a workflow event.
     * @param event Pointer to the workflow event.
     */
    void update(WorkflowEvent* event) override;

    /**
     * @brief Retrieves the recorded messages.
     * @return The recorded messages.
     */
    std::vector<std::string> getRecords();

    /** @brief Destroys the AuditLogger. */
    ~AuditLogger() override;
};

#endif // AUDITLOGGER_H
