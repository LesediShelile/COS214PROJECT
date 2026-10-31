// Tests for the State, Strategy and Chain of Responsibility patterns.
#include "third_party/doctest.h"

#include <ctime>
#include <vector>

#include "Task.h"
#include "Participant.h"
#include "Role.h"
#include "WorkloadAssignment.h"
#include "RoleBasedAssignment.h"
#include "RuleBasedAssignment.h"
#include "ApprovalRequest.h"
#include "ApprovalLevel.h"

// Rule used by the RuleBasedAssignment tests
static bool hasNoTasks(Task*, Participant* p) {
    return p->getWorkload() == 0;
}

// ---------------- State pattern ----------------

TEST_CASE("State: full happy path") {
    Role nurse("Nurse", 1);
    Participant alice("P1", "Alice", &nurse);
    Task task("T1", "Triage", &nurse, nullptr);

    CHECK(task.getStateName() == "Created");
    task.makeAvailable();
    CHECK(task.getStateName() == "Available");
    CHECK(task.isAvailable());
    task.assign(&alice);
    CHECK(task.getStateName() == "Assigned");
    CHECK(task.getAssignee() == &alice);
    task.start();
    CHECK(task.getStateName() == "Started");
    task.complete();
    CHECK(task.getStateName() == "Completed");
    CHECK(task.isFinished());
}

TEST_CASE("State: invalid actions throw and do not change the state") {
    Role nurse("Nurse", 1);
    Participant alice("P1", "Alice", &nurse);
    Task task("T1", "Triage", &nurse, nullptr);

    CHECK_THROWS(task.start());
    CHECK(task.getStateName() == "Created");
    CHECK_THROWS(task.assign(&alice));
    CHECK(task.getStateName() == "Created");

    task.makeAvailable();
    CHECK_THROWS(task.complete());
    CHECK(task.getStateName() == "Available");

    // assign(nullptr) may print or throw depending on the version; either way the state must not change
    try { task.assign(nullptr); } catch (...) {}
    CHECK(task.getStateName() == "Available");

    task.assign(&alice);
    CHECK_THROWS(task.assign(&alice));
    CHECK(task.getStateName() == "Assigned");

    task.start();
    task.complete();
    CHECK_THROWS(task.cancel());
    CHECK(task.getStateName() == "Completed");
}

TEST_CASE("State: reject and make available again") {
    Role nurse("Nurse", 1);
    Participant alice("P1", "Alice", &nurse);
    Task task("T1", "Triage", &nurse, nullptr);

    task.makeAvailable();
    task.assign(&alice);
    task.start();
    task.reject("Missing data");
    CHECK(task.getStateName() == "Rejected");
    CHECK(task.getData("rejectionReason") == "Missing data");
    CHECK(task.getAssignee() == nullptr);

    task.makeAvailable();
    CHECK(task.getStateName() == "Available");
    CHECK(task.getData("rejectionReason") == "");

    task.reject("Not needed");
    CHECK(task.getStateName() == "Rejected");

    Task task2("T2", "Bloods", &nurse, nullptr);
    task2.makeAvailable();
    task2.assign(&alice);
    task2.reject("Wrong person");
    CHECK(task2.getStateName() == "Rejected");
}

TEST_CASE("State: cancel from every state that allows it") {
    Role nurse("Nurse", 1);
    Participant alice("P1", "Alice", &nurse);

    Task created("T1", "A", &nurse, nullptr);
    created.cancel();
    CHECK(created.getStateName() == "Cancelled");

    Task available("T2", "B", &nurse, nullptr);
    available.makeAvailable();
    available.cancel();
    CHECK(available.getStateName() == "Cancelled");

    Task assigned("T3", "C", &nurse, nullptr);
    assigned.makeAvailable();
    assigned.assign(&alice);
    assigned.cancel();
    CHECK(assigned.getStateName() == "Cancelled");

    Task started("T4", "D", &nurse, nullptr);
    started.makeAvailable();
    started.assign(&alice);
    started.start();
    started.cancel();
    CHECK(started.getStateName() == "Cancelled");

    Task rejected("T5", "E", &nurse, nullptr);
    rejected.makeAvailable();
    rejected.reject("x");
    rejected.cancel();
    CHECK(rejected.getStateName() == "Cancelled");
    CHECK(rejected.isFinished());

    CHECK_THROWS(rejected.makeAvailable());
    CHECK(rejected.getStateName() == "Cancelled");
}

TEST_CASE("State: only makeAvailable() makes a task available") {
    Role nurse("Nurse", 1);
    Task first("T1", "First", &nurse, nullptr);
    Task second("T2", "Second", &nurse, nullptr);

    first.makeAvailable();
    CHECK(first.isAvailable());
    CHECK_FALSE(second.isAvailable());

    CHECK_THROWS(second.start());   // a Created task cannot be started
    CHECK_FALSE(second.isAvailable());
}


// ---------------- Strategy pattern ----------------

TEST_CASE("Strategy: assignment strategies") {
    Role nurse("Nurse", 1);
    Role doctor("Doctor", 2);
    Participant busyNurse("P1", "Busy nurse", &nurse);
    Participant freeDoctor("P2", "Free doctor", &doctor);
    Participant freeNurse("P3", "Free nurse", &nurse);
    for (int i = 0; i < 5; i++) busyNurse.incrementWorkload();
    freeNurse.incrementWorkload();
    std::vector<Participant*> everyone = {&busyNurse, &freeDoctor, &freeNurse};

    SUBCASE("Workload: least busy nurse is chosen") {
        WorkloadAssignment workload;
        Task t("T1", "Triage", &nurse, &workload);
        t.makeAvailable();
        t.autoAssign(everyone);
        CHECK(t.getAssignee() == &freeNurse);
    }

    SUBCASE("RoleBased: first nurse in the list is chosen") {
        RoleBasedAssignment roleBased;
        Task t("T2", "Triage", &nurse, &roleBased);
        t.makeAvailable();
        t.autoAssign(everyone);
        CHECK(t.getAssignee() == &busyNurse);
    }

    SUBCASE("RuleBased: function rule and lambda rule") {
        RuleBasedAssignment rules(hasNoTasks);
        Task t("T3", "Anything", nullptr, &rules);
        t.makeAvailable();
        t.autoAssign(everyone);
        CHECK(t.getAssignee() == &freeDoctor);

        int maxLoad = 3;
        rules.setRule([maxLoad](Task* task, Participant* p) {
            return p->hasRole(task->getRequiredRole()) && p->getWorkload() < maxLoad;
        });
        Task t2("T4", "Triage", &nurse, &rules);
        t2.makeAvailable();
        t2.autoAssign(everyone);
        CHECK(t2.getAssignee() == &freeNurse);
    }

    SUBCASE("Nobody suitable: task stays Available") {
        WorkloadAssignment workload;
        Role cleaner("Cleaner", 1);
        Task t("T5", "Clean", &cleaner, &workload);
        t.makeAvailable();
        CHECK_THROWS(t.autoAssign(everyone));
        CHECK(t.getStateName() == "Available");
    }

    SUBCASE("No strategy, then setAssignmentStrategy") {
        RoleBasedAssignment roleBased;
        Task t("T6", "No strategy", &nurse, nullptr);
        t.makeAvailable();
        try {t.autoAssign(everyone); }catch (...) {}
        CHECK(t.getStateName() == "Available") ;

        t.setAssignmentStrategy(&roleBased);
        t.autoAssign(everyone);
        CHECK(t.getStateName() == "Assigned");
    }
}

// ---------------- Chain of Responsibility ----------------

TEST_CASE("Chain of Responsibility: ApprovalRequest") {
    Role doctor("Doctor", 2);
    time_t now = time(nullptr);
    ApprovalRequest request(nullptr, 2, now + 60);

    CHECK(request.getStatus() == ApprovalStatus::Pending);
    CHECK(request.getRequiredLevel() == 2);
    CHECK_FALSE(request.isExpired(now));
    CHECK(request.isExpired(now + 120));

    request.markEscalated();
    CHECK(request.getStatus() == ApprovalStatus::Escalated);
    request.resetDeadline(now + 300);
    CHECK_FALSE(request.isExpired(now + 120));

    request.approve(&doctor);
    CHECK(request.getStatus() == ApprovalStatus::Approved);
    CHECK_FALSE(request.isExpired(now + 999));
    request.reject(&doctor);
    CHECK(request.getStatus() == ApprovalStatus::Approved);
}

TEST_CASE("Chain of Responsibility: approval levels") {
    Role nurse("Nurse", 1);
    Role doctor("Doctor", 2);
    Role head("Head of Department", 3);

    // Each handler owns its successor, so the levels are created with new
    // and deleting the first level deletes the whole chain.
    ApprovalLevel* nurseLevel = new ApprovalLevel(&nurse, 60);
    ApprovalLevel* doctorLevel = new ApprovalLevel(&doctor, 120);
    ApprovalLevel* headLevel = new ApprovalLevel(&head, 300);
    nurseLevel->setNext(doctorLevel);
    doctorLevel->setNext(headLevel);
    time_t now = time(nullptr);

    ApprovalRequest low(nullptr, 1, now);
    nurseLevel->handle(&low);
    CHECK(low.getStatus() == ApprovalStatus::Approved);

    ApprovalRequest mid(nullptr, 2, now);
    nurseLevel->handle(&mid);
    CHECK(mid.getStatus() == ApprovalStatus::Approved);
    CHECK(mid.getDeadline() >= now + 120);

    ApprovalRequest tooHigh(nullptr, 9, now);
    nurseLevel->handle(&tooHigh);
    CHECK(tooHigh.getStatus() == ApprovalStatus::Escalated);

    nurseLevel->handle(&mid);
    CHECK(mid.getStatus() == ApprovalStatus::Approved);

    CHECK_FALSE(nurseLevel->canApprove(&low));
    CHECK(doctorLevel->getRole() == &doctor);
    CHECK(doctorLevel->getTimeout() == 120);

    delete nurseLevel;   // also deletes doctorLevel and headLevel
}