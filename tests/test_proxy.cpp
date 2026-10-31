/**
 * @file test_proxy_pattern.cpp
 * @brief Unit tests for the PROTECTION PROXY pattern (SecureWorkflowProxy).
 *
 * GoF mapping:
 *  - Subject      : IWorkflowService
 *  - RealSubject  : TIVIDYFacade (wrapped, not owned)
 *  - Proxy        : SecureWorkflowProxy (permission + ownership checks)
 *
 * Groups: construction, permission checks, ownership checks, check ordering,
 * forwarding / side-effects, edge cases and polymorphic use.
 */
#include "third_party/doctest.h"
#include "third_party/TestSupport.h"

using namespace testsupport;

// ---------------------------------------------------------------------------
// Construction & lifetime
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: construction and lifetime") {

TEST_CASE("constructing with a null facade throws invalid_argument") {
    CHECK_THROWS_AS(SecureWorkflowProxy(nullptr), std::invalid_argument);
    CHECK(contains(errorOf([] { SecureWorkflowProxy p(nullptr); }), "cannot be null"));
}

TEST_CASE("proxy can be used through the IWorkflowService interface") {
    Fixture f;
    std::unique_ptr<IWorkflowService> service(new SecureWorkflowProxy(&f.facade));
    std::string id = f.newInstance();
    CHECK_NOTHROW(service->startInstance(id, "mgr"));
    CHECK_THROWS_AS(service->startInstance("INST-99", "gina"), std::runtime_error);
}

TEST_CASE("destroying the proxy does NOT destroy the facade it wraps") {
    Fixture f;
    std::string id = f.newInstance();
    {
        SecureWorkflowProxy proxy(&f.facade);
        proxy.startInstance(id, "mgr");
    }   // proxy destroyed here
    // The facade must still be alive and usable (proxy does not own it).
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

TEST_CASE("two proxies over one facade share the same underlying state") {
    Fixture f;
    std::string id = f.newInstance();
    SecureWorkflowProxy p1(&f.facade);
    SecureWorkflowProxy p2(&f.facade);
    p1.startInstance(id, "mgr");
    // second proxy sees that the instance has already been started
    CHECK_THROWS_AS(p2.startInstance(id, "mgr"), std::logic_error);
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Permission checks  (who may perform which operation)
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: permission checks") {

TEST_CASE("startInstance requires 'workflow.start'") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();

    SUBCASE("manager (has permission) is allowed") {
        CHECK_NOTHROW(proxy.startInstance(id, "mgr"));
    }
    SUBCASE("worker (lacks permission) is denied") {
        CHECK_THROWS_AS(proxy.startInstance(id, "alice"), std::runtime_error);
    }
    SUBCASE("guest with no permissions is denied") {
        CHECK_THROWS_AS(proxy.startInstance(id, "gina"), std::runtime_error);
    }
    SUBCASE("admin is allowed") {
        CHECK_NOTHROW(proxy.startInstance(id, "root"));
    }
}

TEST_CASE("assignTask requires 'task.assign'") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();

    CHECK_THROWS_AS(proxy.assignTask(id, "1", "alice", "alice"), std::runtime_error);  // worker
    CHECK_THROWS_AS(proxy.assignTask(id, "1", "alice", "gina"), std::runtime_error);   // guest
    CHECK_NOTHROW(proxy.assignTask(id, "1", "alice", "mgr"));                          // manager
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

TEST_CASE("assignTask does NOT require the caller to own the task (assigner != assignee)") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    proxy.assignTask(id, "1", "bob", "mgr");   // manager assigns to bob
    CHECK(f.facade.isTaskAssignedTo(id, "1", "bob"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "mgr"));
}

TEST_CASE("each task operation needs its own, distinct permission") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);

    // A role that holds ONLY 'task.start'
    Role startOnly("startOnly", 1);
    startOnly.addPermission("task.start");
    f.facade.registerParticipant(new Participant("starter", "Starter", &startOnly));

    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "starter");

    CHECK_NOTHROW(proxy.startTask(id, "1", "starter"));
    // having start does not imply complete / reject / approval
    CHECK(contains(errorOf([&] { proxy.completeTask(id, "1", "starter"); }), "task.complete"));
    CHECK(contains(errorOf([&] { proxy.rejectTask(id, "1", "why", "starter"); }), "task.reject"));
    CHECK(contains(errorOf([&] { proxy.requestApproval(id, "1", "starter"); }), "approval.request"));
}

TEST_CASE("permissions are matched exactly (case sensitive, no prefix matching)") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);

    Role sloppy("sloppy", 1);
    sloppy.addPermission("TASK.ASSIGN");   // wrong case
    sloppy.addPermission("task");          // prefix only
    sloppy.addPermission("task.assign ");  // trailing space
    f.facade.registerParticipant(new Participant("sloppy", "Sloppy", &sloppy));

    std::string id = f.startedInstance();
    CHECK_THROWS_AS(proxy.assignTask(id, "1", "alice", "sloppy"), std::runtime_error);
}

TEST_CASE("a permission granted to a role at runtime takes effect immediately") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();

    CHECK_THROWS_AS(proxy.startInstance(id, "gina"), std::runtime_error);
    f.guestRole.addPermission("workflow.start");   // dynamic grant
    CHECK_NOTHROW(proxy.startInstance(id, "gina"));
}

TEST_CASE("participants sharing a role share its permissions") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 2));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.assignTask(id, "2", "bob");
    // alice and bob hold the same role, so both may start their own task
    CHECK_NOTHROW(proxy.startTask(id, "1", "alice"));
    CHECK_NOTHROW(proxy.startTask(id, "2", "bob"));
}

TEST_CASE("denial message names the user and the missing permission") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    std::string msg = errorOf([&] { proxy.startInstance(id, "gina"); });
    CHECK(contains(msg, "Access denied"));
    CHECK(contains(msg, "gina"));
    CHECK(contains(msg, "workflow.start"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Unknown / malformed users
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: unknown and malformed users") {

TEST_CASE("an unregistered user is denied (not an invalid_argument leak)") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    // hasPermission() swallows the facade's invalid_argument -> Access denied
    CHECK_THROWS_AS(proxy.startInstance(id, "ghost"), std::runtime_error);
    CHECK(contains(errorOf([&] { proxy.startInstance(id, "ghost"); }), "Access denied"));
}

TEST_CASE("an empty user ID is denied") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    CHECK_THROWS_AS(proxy.startInstance(id, ""), std::runtime_error);
}

TEST_CASE("user IDs are case sensitive") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    CHECK_THROWS_AS(proxy.startInstance(id, "MGR"), std::runtime_error);
    CHECK_THROWS_AS(proxy.startInstance(id, "Mgr"), std::runtime_error);
    CHECK_NOTHROW(proxy.startInstance(id, "mgr"));
}

TEST_CASE("user IDs with surrounding whitespace are not trimmed") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    CHECK_THROWS_AS(proxy.startInstance(id, " mgr"), std::runtime_error);
    CHECK_THROWS_AS(proxy.startInstance(id, "mgr "), std::runtime_error);
}

TEST_CASE("a participant whose Role* is nullptr is denied everything") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(proxy.startInstance(id, "norole"), std::runtime_error);
    CHECK_THROWS_AS(proxy.assignTask(id, "1", "alice", "norole"), std::runtime_error);
    CHECK_THROWS_AS(proxy.startTask(id, "1", "norole"), std::runtime_error);
    CHECK_THROWS_AS(proxy.completeTask(id, "1", "norole"), std::runtime_error);
    CHECK_THROWS_AS(proxy.rejectTask(id, "1", "x", "norole"), std::runtime_error);
    CHECK_THROWS_AS(proxy.requestApproval(id, "1", "norole"), std::runtime_error);
}

TEST_CASE("a user registered AFTER the proxy was created is recognised") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    Role late("late", 1);
    late.addPermission("workflow.start");
    CHECK_THROWS_AS(proxy.startInstance(id, "late"), std::runtime_error);
    f.facade.registerParticipant(new Participant("late", "Latecomer", &late));
    CHECK_NOTHROW(proxy.startInstance(id, "late"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Ownership checks
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: task ownership checks") {

TEST_CASE("startTask / completeTask / rejectTask / requestApproval need ownership") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");

    // bob has every permission alice has, but is NOT the assignee
    std::string m1 = errorOf([&] { proxy.startTask(id, "1", "bob"); });
    CHECK(contains(m1, "Access denied"));
    CHECK(contains(m1, "bob"));
    CHECK(contains(m1, "not assigned"));
    CHECK(contains(m1, "'1'"));
    CHECK(contains(errorOf([&] { proxy.completeTask(id, "1", "bob"); }), "not assigned"));
    CHECK(contains(errorOf([&] { proxy.rejectTask(id, "1", "r", "bob"); }), "not assigned"));
    CHECK(contains(errorOf([&] { proxy.requestApproval(id, "1", "bob"); }), "not assigned"));
}

TEST_CASE("the real assignee is allowed") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    CHECK_NOTHROW(proxy.startTask(id, "1", "alice"));
    CHECK_NOTHROW(proxy.completeTask(id, "1", "alice"));
}

TEST_CASE("admin has every permission but still cannot act on a task it does not own") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    CHECK_THROWS_AS(proxy.startTask(id, "1", "root"), std::runtime_error);
    CHECK(contains(errorOf([&] { proxy.startTask(id, "1", "root"); }), "not assigned"));
}

TEST_CASE("an unassigned (Available) task is owned by nobody") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(proxy.startTask(id, "1", "alice"), std::runtime_error);
    CHECK(contains(errorOf([&] { proxy.startTask(id, "1", "alice"); }), "not assigned"));
}

TEST_CASE("ownership is per instance: assignee of INST-1 does not own the same task in INST-2") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    std::string a = f.facade.createInstance("seq");
    std::string b = f.facade.createInstance("seq");
    f.facade.startInstance(a);
    f.facade.startInstance(b);
    f.facade.assignTask(a, "1", "alice");
    f.facade.assignTask(b, "1", "bob");

    CHECK_NOTHROW(proxy.startTask(a, "1", "alice"));
    CHECK_THROWS_AS(proxy.startTask(b, "1", "alice"), std::runtime_error);  // alice not on B
    CHECK_NOTHROW(proxy.startTask(b, "1", "bob"));
}

TEST_CASE("after a task is rejected the former owner loses ownership") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    proxy.startTask(id, "1", "alice");
    proxy.rejectTask(id, "1", "bad data", "alice");   // clears the assignee
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    // ownership is checked BEFORE the state machine, so this is Access denied,
    // not a logic_error about the Rejected state.
    std::string msg = errorOf([&] { proxy.completeTask(id, "1", "alice"); });
    CHECK(contains(msg, "Access denied"));
    CHECK(contains(msg, "not assigned"));
}

TEST_CASE("a Stage is never owned by a user") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    // item "0" is the root SequentialStage
    CHECK_THROWS_AS(proxy.startTask(id, "0", "alice"), std::runtime_error);
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Order of checks: permission -> ownership -> real service
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: order of checks and information hiding") {

TEST_CASE("permission is checked before the instance is looked up") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    // unauthorised caller + nonexistent instance => Access denied, not invalid_argument
    CHECK_THROWS_AS(proxy.startInstance("INST-404", "gina"), std::runtime_error);
    CHECK(contains(errorOf([&] { proxy.startInstance("INST-404", "gina"); }), "Access denied"));
}

TEST_CASE("authorised caller + nonexistent instance => facade's invalid_argument is forwarded") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    CHECK_THROWS_AS(proxy.startInstance("INST-404", "mgr"), std::invalid_argument);
}

TEST_CASE("permission is checked before ownership") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    // gina is neither permitted nor owner: the PERMISSION failure must be reported
    std::string msg = errorOf([&] { proxy.startTask(id, "1", "gina"); });
    CHECK(contains(msg, "does not have permission 'task.start'"));
}

TEST_CASE("ownership is checked before the state machine") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    // Task1 is Assigned, so complete would be an invalid transition, but a
    // non-owner must be told 'not assigned' instead.
    CHECK_THROWS_AS(proxy.completeTask(id, "1", "bob"), std::runtime_error);
    CHECK(contains(errorOf([&] { proxy.completeTask(id, "1", "bob"); }), "not assigned"));
}

TEST_CASE("authorised owner + unknown task => facade's invalid_argument from ownership lookup") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(proxy.startTask(id, "999", "alice"), std::invalid_argument);
}

TEST_CASE("authorised owner + unknown instance => invalid_argument") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    CHECK_THROWS_AS(proxy.startTask("INST-404", "1", "alice"), std::invalid_argument);
}

TEST_CASE("unauthorised caller + unknown task => Access denied (no existence leak)") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(proxy.startTask(id, "999", "gina"), std::runtime_error);
    CHECK(contains(errorOf([&] { proxy.startTask(id, "999", "gina"); }), "Access denied"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Forwarding and side effects
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: forwarding and side effects") {

TEST_CASE("a denied call has no side effects on the real service") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    const std::size_t before = f.facade.getAuditLog().size();

    CHECK_THROWS(proxy.assignTask(id, "1", "alice", "gina"));
    CHECK_THROWS(proxy.startTask(id, "1", "gina"));

    CHECK(f.facade.getAuditLog().size() == before);          // nothing was logged
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice")); // nothing was assigned
    CHECK_FALSE(f.facade.undoLastAction());                   // no command was executed
}

TEST_CASE("an allowed call reaches the facade and changes state") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    proxy.assignTask(id, "1", "alice", "mgr");
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK(countContaining(f.facade.getAuditLog(), "'Task1' was assigned") == 1);
}

TEST_CASE("the full lifecycle can be driven through the proxy") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.newInstance();
    proxy.startInstance(id, "mgr");
    proxy.assignTask(id, "1", "alice", "mgr");
    proxy.startTask(id, "1", "alice");
    proxy.completeTask(id, "1", "alice");
    std::vector<std::string> log = f.facade.getAuditLog();
    CHECK(countContaining(log, "was assigned (now Assigned)") == 1);
    CHECK(countContaining(log, "was started (now Started)") == 1);
    CHECK(countContaining(log, "was completed (now Completed)") == 1);
}

TEST_CASE("rejectTask forwards the reason unchanged, including an empty reason") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 2));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.assignTask(id, "2", "bob");

    proxy.rejectTask(id, "1", "missing signature", "alice");
    proxy.rejectTask(id, "2", "", "bob");
    std::vector<std::string> log = f.facade.getAuditLog();
    CHECK(countContaining(log, "was rejected: missing signature") == 1);
    CHECK(countContaining(log, "'Task2' was rejected: ") == 1);
}

TEST_CASE("business-rule failures of the real service propagate through an authorised call") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    proxy.assignTask(id, "1", "alice", "mgr");
    // alice is permitted and owns the task, but Assigned -> Completed is illegal
    CHECK_THROWS_AS(proxy.completeTask(id, "1", "alice"), std::logic_error);
    // double start
    proxy.startTask(id, "1", "alice");
    CHECK_THROWS_AS(proxy.startTask(id, "1", "alice"), std::logic_error);
    // authorised double start of the instance
    CHECK_THROWS_AS(proxy.startInstance(id, "mgr"), std::logic_error);
}

TEST_CASE("a successful proxy call is undoable through the facade (Command still works)") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    proxy.assignTask(id, "1", "alice", "mgr");
    REQUIRE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK(f.facade.undoLastAction());
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// requestApproval through the proxy
// ---------------------------------------------------------------------------
TEST_SUITE("Proxy: requestApproval") {

TEST_CASE("authorised owner receives the facade's ApprovalStatus") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    WorkflowDefinition* def = makeSequentialDefinition("appr", &f.workerRole, 1);
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("appr");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    CHECK(proxy.requestApproval(id, "1", "alice") == ApprovalStatus::Approved);
}

TEST_CASE("without an approval chain the facade's runtime_error is forwarded (not 'Access denied')") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    std::string msg = errorOf([&] { proxy.requestApproval(id, "1", "alice"); });
    CHECK_FALSE(msg.empty());
    CHECK_FALSE(contains(msg, "Access denied"));
    CHECK(contains(msg, "approval chain"));
}

TEST_CASE("an unauthorised caller cannot trigger the approval chain") {
    Fixture f;
    SecureWorkflowProxy proxy(&f.facade);
    WorkflowDefinition* def = makeSequentialDefinition("appr", &f.workerRole, 1);
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("appr");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    CHECK_THROWS_AS(proxy.requestApproval(id, "1", "gina"), std::runtime_error);
    CHECK_THROWS_AS(proxy.requestApproval(id, "1", "bob"), std::runtime_error);  // not owner
}

} // TEST_SUITE