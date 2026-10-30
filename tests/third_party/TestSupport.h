/**
 * @file TestSupport.h
 * @brief Shared fixtures and helpers for the Proxy / Facade test-suites.
 *
 * Ownership rules the fixtures respect (they mirror the production code):
 *  - TIVIDYFacade OWNS every Participant and WorkflowDefinition registered
 *    with it, so tests allocate them with `new` and never delete them.
 *  - Roles are NOT owned by Participant or by the facade; the Fixture owns them.
 *  - A WorkflowDefinition owns its approval chain; ApprovalHandler deletes its
 *    `next` handler, so a chain is built with `new` and handed over whole.
 *  - If registration is REJECTED (duplicate / null) the caller keeps ownership.
 */
#ifndef TIVIDY_TEST_SUPPORT_H
#define TIVIDY_TEST_SUPPORT_H

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/ApprovalLevel.h"
#include "../../include/ApprovalStatus.h"
#include "../../include/DefinitionBuilder.h"
#include "../../include/Participant.h"
#include "../../include/Role.h"
#include "../../include/SecureWorkflowProxy.h"
#include "../../include/WorkItem.h"
#include "../../include/TIVIDYFacade.h"
#include "../../include/WorkflowDefinition.h"

namespace testsupport {

/** @brief Runs @p f and returns what() of the exception it throws, or "" if none. */
template <typename F>
std::string errorOf(F f) {
    try {
        f();
    } catch (const std::exception& e) {
        return e.what();
    }
    return "";
}

/** @brief True when @p haystack contains @p needle. */
inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

/** @brief Number of audit records that contain @p needle. */
inline int countContaining(const std::vector<std::string>& log, const std::string& needle) {
    int n = 0;
    for (std::size_t i = 0; i < log.size(); ++i) {
        if (contains(log[i], needle)) ++n;
    }
    return n;
}

/**
 * @brief Builds a root SequentialStage "Flow" holding @p taskCount audited tasks.
 *
 * Item IDs are deterministic: the root stage is "0", tasks are "1", "2", ...
 * Tasks are named "Task1", "Task2", ... and wrapped in AuditedTask so every
 * lifecycle change appears in the facade's audit log.
 */
inline WorkflowDefinition* makeSequentialDefinition(const std::string& name, Role* role,
                                                    int taskCount, bool audited = true) {
    DefinitionBuilder b;
    b.reset(name, 1);
    b.beginSequentialStage("Flow");
    for (int i = 1; i <= taskCount; ++i) {
        b.addTask("Task" + std::to_string(i), role, nullptr);
        if (audited) b.withAudit(nullptr);
        b.commitPendingTask();
    }
    b.endStage();
    return b.getResult();
}

/** @brief Same as above but the root stage is a ParallelStage (all tasks open at once). */
inline WorkflowDefinition* makeParallelDefinition(const std::string& name, Role* role,
                                                  int taskCount) {
    DefinitionBuilder b;
    b.reset(name, 1);
    b.beginParallelStage("Flow");
    for (int i = 1; i <= taskCount; ++i) {
        b.addTask("Task" + std::to_string(i), role, nullptr);
        b.withAudit(nullptr);
        b.commitPendingTask();
    }
    b.endStage();
    return b.getResult();
}

/** @brief Gives every task directly under the root a data key/value (before registration). */
inline void tagTasks(WorkflowDefinition* def, const std::string& key, const std::string& value) {
    std::vector<WorkItem*> kids = def->getRoot()->getChildren();
    for (std::size_t i = 0; i < kids.size(); ++i) kids[i]->setData(key, value);
}

/**
 * @brief Approval chain Nurse(authority 1) -> Doctor(authority 3), heap allocated.
 * @param nurse  role with authority 1
 * @param doctor role with authority 3
 */
inline ApprovalLevel* makeApprovalChain(Role* nurse, Role* doctor) {
    ApprovalLevel* first = new ApprovalLevel(nurse, 3600);
    first->setNext(new ApprovalLevel(doctor, 3600));
    return first;
}

/**
 * @brief Standard cast of characters used by most tests.
 *
 * Roles / permissions:
 *  - manager : workflow.start, task.assign, approval.request
 *  - worker  : task.start, task.complete, task.reject, approval.request
 *  - admin   : every permission
 *  - guest   : no permissions at all
 *
 * Participants (all registered with the facade, which owns them):
 *  mgr (manager), alice (worker), bob (worker), root (admin), gina (guest),
 *  norole (participant whose Role* is nullptr)
 */
struct Fixture {
    Role managerRole;
    Role workerRole;
    Role adminRole;
    Role guestRole;
    Role nurseRole;   ///< authority 1, used for approval chains
    Role doctorRole;  ///< authority 3, used for approval chains
    TIVIDYFacade facade;

    Fixture()
        : managerRole("manager", 2), workerRole("worker", 1), adminRole("admin", 5),
          guestRole("guest", 0), nurseRole("nurse", 1), doctorRole("doctor", 3) {
        managerRole.addPermission("workflow.start");
        managerRole.addPermission("task.assign");
        managerRole.addPermission("approval.request");

        workerRole.addPermission("task.start");
        workerRole.addPermission("task.complete");
        workerRole.addPermission("task.reject");
        workerRole.addPermission("approval.request");

        const char* all[] = {"workflow.start", "task.assign", "task.start",
                             "task.complete",  "task.reject", "approval.request"};
        for (int i = 0; i < 6; ++i) adminRole.addPermission(all[i]);

        facade.registerParticipant(new Participant("mgr", "Mia Manager", &managerRole));
        facade.registerParticipant(new Participant("alice", "Alice Worker", &workerRole));
        facade.registerParticipant(new Participant("bob", "Bob Worker", &workerRole));
        facade.registerParticipant(new Participant("root", "Root Admin", &adminRole));
        facade.registerParticipant(new Participant("gina", "Gina Guest", &guestRole));
        facade.registerParticipant(new Participant("norole", "No Role", nullptr));
    }

    /** @brief Registers a sequential definition and returns a fresh, NOT started instance ID. */
    std::string newInstance(const std::string& defName = "seq", int tasks = 2) {
        facade.registerDefinition(makeSequentialDefinition(defName, &workerRole, tasks));
        return facade.createInstance(defName);
    }

    /** @brief Registers a sequential definition, creates and starts an instance. */
    std::string startedInstance(const std::string& defName = "seq", int tasks = 2) {
        std::string id = newInstance(defName, tasks);
        facade.startInstance(id);
        return id;
    }
};

} // namespace testsupport

#endif