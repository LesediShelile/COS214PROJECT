#ifndef TIVIDYFACADE_H
#define TIVIDYFACADE_H

#include <map>
#include <string>
#include <vector>

#include "ApprovalStatus.h"
#include "AuditLogger.h"
#include "CommandInvoker.h"
#include "MetricsCollector.h"
#include "Notifier.h"

class WorkflowDefinition;
class WorkflowInstance;
class WorkItem;
class Participant;
class EngineRouter;

/**
 * Facade: Our single entry point to TIVIDY
 *
 * Clients register definitions and participants, create and start instances,
 * and act on work items by ID. 
 * They never touch the builders, iterators,
 * commands, listeners or approval chain directly.
 *
 * The facade owns every definition, instance, participant and router registered 
 * with or created by it. 
 * Listeners (audit, notifier, metrics) are members, and instances only hold
 * non-owning pointers to them.
 */


class TIVIDYFacade {
public:
    TIVIDYFacade();
    ~TIVIDYFacade();

    
    TIVIDYFacade(const TIVIDYFacade&) = delete;
    TIVIDYFacade& operator=(const TIVIDYFacade&) = delete;

    // The facade takes ownership on success; if an exception is thrown,
    // the caller keeps ownership of the pointer.
    void registerDefinition(WorkflowDefinition* definition);
    void registerParticipant(Participant* participant);

    // Instance lifecycle.
    std::string createInstance(const std::string& definitionName);
    void startInstance(const std::string& instanceID);

    // Actions on work items (each is executed as a Command, so it can be undone).
    void assignTask(const std::string& instanceID, const std::string& taskID,
                    const std::string& participantID);
    void completeTask(const std::string& instanceID, const std::string& taskID);
    void rejectTask(const std::string& instanceID, const std::string& taskID,
                    const std::string& reason);
    bool undoLastAction();

    // Approval and monitoring.
    ApprovalStatus requestApproval(const std::string& instanceID,
                                   const std::string& taskID);
    std::string getMetricsSummary();
    std::vector<std::string> getAuditLog();

private:
    WorkItem* findItem(const std::string& instanceID, const std::string& itemID);
    WorkflowInstance* findInstance(const std::string& instanceID);
    Participant* findParticipant(const std::string& participantID);

    std::map<std::string, WorkflowDefinition*> definitions;
    std::map<std::string, WorkflowInstance*> instances;
    std::map<std::string, Participant*> participants;
    std::vector<EngineRouter*> routers;
    CommandInvoker invoker;
    AuditLogger audit;
    Notifier notifier;
    MetricsCollector metrics;
    int nextInstanceNumber;
};

#endif