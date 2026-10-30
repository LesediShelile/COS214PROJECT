/**
 * @file test_facade_pattern.cpp
 * @brief Unit tests for the FACADE pattern (TIVIDYFacade).
 *
 * GoF mapping:
 *  - Facade     : TIVIDYFacade
 *  - Subsystems : WorkflowDefinition / WorkflowInstance / WorkItem tree,
 *                 Participant, CommandInvoker, EventPublisher + listeners,
 *                 ApprovalHandler chain
 *
 * A client should be able to do everything with string IDs only and never touch
 * the subsystem classes. These tests exercise the facade's public API: its
 * registration rules, ID lookup, lifecycle operations, error handling and
 * instance isolation. (Command / Observer / Chain-of-Responsibility behaviour
 * reached THROUGH the facade is covered in the test_facade_*.cpp siblings.)
 */
#include "third_party/doctest.h"
#include "third_party/TestSupport.h"

using namespace testsupport;

// ---------------------------------------------------------------------------
// Registering definitions
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: registerDefinition") {

TEST_CASE("null definition is rejected") {
    TIVIDYFacade facade;
    CHECK_THROWS_AS(facade.registerDefinition(nullptr), std::invalid_argument);
}

TEST_CASE("a valid definition is registered, frozen and can create instances") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("seq", &f.workerRole, 1);
    REQUIRE_FALSE(def->isFrozen());
    f.facade.registerDefinition(def);   // facade now owns def
    CHECK(def->isFrozen());
    CHECK_NOTHROW(f.facade.createInstance("seq"));
}

TEST_CASE("registering a duplicate name is rejected and the caller keeps ownership") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("dup", &f.workerRole, 1));
    std::unique_ptr<WorkflowDefinition> second(makeSequentialDefinition("dup", &f.workerRole, 2));
    CHECK_THROWS_AS(f.facade.registerDefinition(second.get()), std::invalid_argument);
    CHECK(contains(errorOf([&] { f.facade.registerDefinition(second.get()); }), "already registered"));
    CHECK_FALSE(second->isFrozen());   // rejected before it was frozen
}

TEST_CASE("duplicate detection is by NAME only (a different version does not help)") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("flow", &f.workerRole, 1));
    DefinitionBuilder b;
    b.reset("flow", 2);   // version 2, same name
    b.beginSequentialStage("S");
    b.endStage();
    std::unique_ptr<WorkflowDefinition> v2(b.getResult());
    CHECK_THROWS_AS(f.facade.registerDefinition(v2.get()), std::invalid_argument);
}

TEST_CASE("registering the very same pointer twice is rejected") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("same", &f.workerRole, 1);
    f.facade.registerDefinition(def);
    CHECK_THROWS_AS(f.facade.registerDefinition(def), std::invalid_argument);
    // facade must still own it exactly once (ASan would flag a double free)
    CHECK_NOTHROW(f.facade.createInstance("same"));
}

TEST_CASE("different names register side by side") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("a", &f.workerRole, 1));
    f.facade.registerDefinition(makeSequentialDefinition("b", &f.workerRole, 1));
    CHECK_NOTHROW(f.facade.createInstance("a"));
    CHECK_NOTHROW(f.facade.createInstance("b"));
}

TEST_CASE("definition names are case sensitive") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("Flow", &f.workerRole, 1));
    f.facade.registerDefinition(makeSequentialDefinition("flow", &f.workerRole, 1));
    CHECK_NOTHROW(f.facade.createInstance("Flow"));
    CHECK_NOTHROW(f.facade.createInstance("flow"));
    CHECK_THROWS_AS(f.facade.createInstance("FLOW"), std::invalid_argument);
}

TEST_CASE("an already-frozen definition is accepted as is") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("pre", &f.workerRole, 1);
    def->freeze();
    CHECK_NOTHROW(f.facade.registerDefinition(def));
    CHECK(def->isFrozen());
}

TEST_CASE("a definition with no root cannot be registered (freeze fails)") {
    Fixture f;
    std::unique_ptr<WorkflowDefinition> empty(new WorkflowDefinition("empty", 1));
    CHECK_THROWS_AS(f.facade.registerDefinition(empty.get()), std::logic_error);
    // ...and it was NOT registered
    CHECK_THROWS_AS(f.facade.createInstance("empty"), std::invalid_argument);
}

TEST_CASE("a registered definition can no longer be modified") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("locked", &f.workerRole, 1);
    f.facade.registerDefinition(def);
    CHECK_THROWS_AS(def->setApprovalChain(nullptr), std::logic_error);
}

TEST_CASE("an empty-named definition can be registered and instantiated") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("", &f.workerRole, 1));
    CHECK_NOTHROW(f.facade.createInstance(""));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Registering / looking up participants
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: participants") {

TEST_CASE("null participant is rejected") {
    TIVIDYFacade facade;
    CHECK_THROWS_AS(facade.registerParticipant(nullptr), std::invalid_argument);
}

TEST_CASE("registered participant can be fetched by ID and is the same object") {
    TIVIDYFacade facade;
    Role r("r", 1);
    Participant* p = new Participant("p1", "Pat", &r);
    facade.registerParticipant(p);
    const Participant* got = facade.getParticipant("p1");
    CHECK(got == p);
    CHECK(got->getName() == "Pat");
    CHECK(got->getRole() == &r);
}

TEST_CASE("duplicate participant ID is rejected and the caller keeps ownership") {
    TIVIDYFacade facade;
    Role r("r", 1);
    facade.registerParticipant(new Participant("p1", "First", &r));
    std::unique_ptr<Participant> dup(new Participant("p1", "Second", &r));
    CHECK_THROWS_AS(facade.registerParticipant(dup.get()), std::invalid_argument);
    CHECK(contains(errorOf([&] { facade.registerParticipant(dup.get()); }), "already registered"));
    // the ORIGINAL is untouched
    CHECK(facade.getParticipant("p1")->getName() == "First");
}

TEST_CASE("registering the same participant pointer twice is rejected") {
    TIVIDYFacade facade;
    Role r("r", 1);
    Participant* p = new Participant("p1", "Pat", &r);
    facade.registerParticipant(p);
    CHECK_THROWS_AS(facade.registerParticipant(p), std::invalid_argument);
}

TEST_CASE("getParticipant on an unknown ID throws invalid_argument naming the ID") {
    TIVIDYFacade facade;
    CHECK_THROWS_AS(facade.getParticipant("nobody"), std::invalid_argument);
    CHECK(contains(errorOf([&] { facade.getParticipant("nobody"); }), "nobody"));
}

TEST_CASE("getParticipant is case sensitive and does not trim") {
    TIVIDYFacade facade;
    Role r("r", 1);
    facade.registerParticipant(new Participant("Pat", "Pat", &r));
    CHECK_THROWS_AS(facade.getParticipant("pat"), std::invalid_argument);
    CHECK_THROWS_AS(facade.getParticipant("Pat "), std::invalid_argument);
    CHECK_NOTHROW(facade.getParticipant("Pat"));
}

TEST_CASE("getParticipant works on a const facade") {
    TIVIDYFacade facade;
    Role r("r", 1);
    facade.registerParticipant(new Participant("p1", "Pat", &r));
    const TIVIDYFacade& cref = facade;
    CHECK(cref.getParticipant("p1")->getParticipantID() == "p1");
}

TEST_CASE("an empty participant ID is accepted as a (strange) valid key") {
    TIVIDYFacade facade;
    Role r("r", 1);
    CHECK_NOTHROW(facade.registerParticipant(new Participant("", "Anon", &r)));
    CHECK(facade.getParticipant("")->getName() == "Anon");
}

TEST_CASE("many participants can be registered") {
    TIVIDYFacade facade;
    Role r("r", 1);
    for (int i = 0; i < 200; ++i) {
        facade.registerParticipant(new Participant("p" + std::to_string(i), "n", &r));
    }
    CHECK(facade.getParticipant("p0")->getParticipantID() == "p0");
    CHECK(facade.getParticipant("p199")->getParticipantID() == "p199");
    CHECK_THROWS_AS(facade.getParticipant("p200"), std::invalid_argument);
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// createInstance / startInstance
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: createInstance and startInstance") {

TEST_CASE("instance IDs are sequential: INST-1, INST-2, ...") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    CHECK(f.facade.createInstance("seq") == "INST-1");
    CHECK(f.facade.createInstance("seq") == "INST-2");
    CHECK(f.facade.createInstance("seq") == "INST-3");
}

TEST_CASE("the counter is shared across definitions") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("a", &f.workerRole, 1));
    f.facade.registerDefinition(makeSequentialDefinition("b", &f.workerRole, 1));
    CHECK(f.facade.createInstance("a") == "INST-1");
    CHECK(f.facade.createInstance("b") == "INST-2");
}

TEST_CASE("unknown definition name throws and does not consume an instance number") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    CHECK_THROWS_AS(f.facade.createInstance("nope"), std::invalid_argument);
    CHECK(contains(errorOf([&] { f.facade.createInstance("nope"); }), "nope"));
    CHECK(f.facade.createInstance("seq") == "INST-1");   // no gap
}

TEST_CASE("createInstance on an empty facade throws") {
    TIVIDYFacade facade;
    CHECK_THROWS_AS(facade.createInstance("anything"), std::invalid_argument);
}

TEST_CASE("a new instance is not started: its first task is not yet available") {
    Fixture f;
    std::string id = f.newInstance();
    CHECK(f.facade.getAuditLog().empty());
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "alice"), std::logic_error);
}

TEST_CASE("startInstance makes the first task available (and only the first, sequentially)") {
    Fixture f;
    std::string id = f.startedInstance("seq", 3);
    std::vector<std::string> log = f.facade.getAuditLog();
    CHECK(countContaining(log, "'Task1' became available") == 1);
    CHECK(countContaining(log, "'Task2' became available") == 0);
    CHECK(countContaining(log, "'Task3' became available") == 0);
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));
    CHECK_THROWS_AS(f.facade.assignTask(id, "2", "alice"), std::logic_error);
}

TEST_CASE("startInstance opens every task of a parallel stage") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 3));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    std::vector<std::string> log = f.facade.getAuditLog();
    CHECK(countContaining(log, "became available") == 3);
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));
    CHECK_NOTHROW(f.facade.assignTask(id, "2", "alice"));
    CHECK_NOTHROW(f.facade.assignTask(id, "3", "bob"));
}

TEST_CASE("startInstance on an unknown instance throws invalid_argument") {
    Fixture f;
    CHECK_THROWS_AS(f.facade.startInstance("INST-1"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.startInstance(""), std::invalid_argument);
}

TEST_CASE("starting an instance twice throws logic_error and does not re-fire events") {
    Fixture f;
    std::string id = f.startedInstance();
    const std::size_t before = f.facade.getAuditLog().size();
    CHECK_THROWS_AS(f.facade.startInstance(id), std::logic_error);
    CHECK(f.facade.getAuditLog().size() == before);
}

TEST_CASE("an instance whose stage has no tasks starts without error") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("hollow", &f.workerRole, 0));
    std::string id = f.facade.createInstance("hollow");
    CHECK_NOTHROW(f.facade.startInstance(id));
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "alice"), std::invalid_argument);  // no such task
}

TEST_CASE("many instances of one definition can coexist") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 2));
    for (int i = 1; i <= 100; ++i) {
        CHECK(f.facade.createInstance("seq") == "INST-" + std::to_string(i));
    }
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Lookup / ID error handling (findInstance / findItem / findParticipant)
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: ID lookup and error handling") {

TEST_CASE("every task operation rejects an unknown instance with invalid_argument") {
    Fixture f;
    CHECK_THROWS_AS(f.facade.assignTask("INST-9", "1", "alice"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.startTask("INST-9", "1"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.completeTask("INST-9", "1"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.rejectTask("INST-9", "1", "r"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.requestApproval("INST-9", "1"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.isTaskAssignedTo("INST-9", "1", "alice"), std::invalid_argument);
    CHECK(contains(errorOf([&] { f.facade.startTask("INST-9", "1"); }), "unknown instance"));
}

TEST_CASE("every task operation rejects an unknown task with invalid_argument") {
    Fixture f;
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(f.facade.assignTask(id, "42", "alice"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.startTask(id, "42"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.completeTask(id, "42"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.rejectTask(id, "42", "r"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.requestApproval(id, "42"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.isTaskAssignedTo(id, "42", "alice"), std::invalid_argument);
    CHECK(contains(errorOf([&] { f.facade.startTask(id, "42"); }), "no item '42'"));
}

TEST_CASE("assignTask rejects an unknown participant with invalid_argument") {
    Fixture f;
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "ghost"), std::invalid_argument);
    CHECK(contains(errorOf([&] { f.facade.assignTask(id, "1", "ghost"); }), "ghost"));
    // the task was left untouched and is still assignable
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));
}

TEST_CASE("unknown instance is reported before unknown task or participant") {
    Fixture f;
    std::string msg = errorOf([&] { f.facade.assignTask("INST-9", "42", "ghost"); });
    CHECK(contains(msg, "unknown instance"));
}

TEST_CASE("unknown task is reported before unknown participant") {
    Fixture f;
    std::string id = f.startedInstance();
    std::string msg = errorOf([&] { f.facade.assignTask(id, "42", "ghost"); });
    CHECK(contains(msg, "no item '42'"));
}

TEST_CASE("item IDs are matched exactly: '01', ' 1', '1 ', '' and 'Task1' are not '1'") {
    Fixture f;
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(f.facade.assignTask(id, "01", "alice"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.assignTask(id, " 1", "alice"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.assignTask(id, "1 ", "alice"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.assignTask(id, "", "alice"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.assignTask(id, "Task1", "alice"), std::invalid_argument);   // name, not ID
}

TEST_CASE("instance IDs are matched exactly (case, whitespace)") {
    Fixture f;
    std::string id = f.startedInstance();   // "INST-1"
    CHECK_THROWS_AS(f.facade.startTask("inst-1", "1"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.startTask(id + " ", "1"), std::invalid_argument);
}

TEST_CASE("the root stage can be found by ID but is not a workable task") {
    Fixture f;
    std::string id = f.startedInstance();
    // item "0" is the Stage: lookup succeeds, the operation itself is illegal
    CHECK_THROWS_AS(f.facade.assignTask(id, "0", "alice"), std::logic_error);
    CHECK_THROWS_AS(f.facade.startTask(id, "0"), std::logic_error);
    CHECK_THROWS_AS(f.facade.completeTask(id, "0"), std::logic_error);
    CHECK_THROWS_AS(f.facade.rejectTask(id, "0", "r"), std::logic_error);
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "0", "alice"));
}

TEST_CASE("exceptions are of the documented types: invalid_argument for IDs, logic_error for state") {
    Fixture f;
    std::string id = f.startedInstance();
    bool sawInvalidArgument = false, sawLogicError = false;
    try { f.facade.startTask("nope", "1"); } catch (const std::invalid_argument&) { sawInvalidArgument = true; }
    try { f.facade.completeTask(id, "1"); } catch (const std::logic_error&) { sawLogicError = true; }
    CHECK(sawInvalidArgument);
    CHECK(sawLogicError);
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// isTaskAssignedTo
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: isTaskAssignedTo") {

TEST_CASE("false before assignment, true for the assignee only afterwards") {
    Fixture f;
    std::string id = f.startedInstance();
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    f.facade.assignTask(id, "1", "alice");
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "bob"));
}

TEST_CASE("an unknown participant ID simply yields false (it is only compared, not looked up)") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "ghost"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", ""));
}

TEST_CASE("assignment is per task") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 2));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "2", "alice"));
}

TEST_CASE("assignee survives start and complete") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
    f.facade.completeTask(id, "1");
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

TEST_CASE("rejecting an Assigned or Started task clears the assignee") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 2));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.assignTask(id, "2", "bob");
    f.facade.startTask(id, "2");

    f.facade.rejectTask(id, "1", "from Assigned");
    f.facade.rejectTask(id, "2", "from Started");
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "2", "bob"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Task lifecycle through the facade (State machine reached via the facade)
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: task lifecycle transitions") {

TEST_CASE("happy path Available -> Assigned -> Started -> Completed") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    f.facade.completeTask(id, "1");
    std::vector<std::string> log = f.facade.getAuditLog();
    REQUIRE(log.size() == 4);
    CHECK(contains(log[0], "became available (now Available)"));
    CHECK(contains(log[1], "was assigned (now Assigned)"));
    CHECK(contains(log[2], "was started (now Started)"));
    CHECK(contains(log[3], "was completed (now Completed)"));
}

TEST_CASE("illegal transitions throw logic_error and leave the task usable") {
    Fixture f;
    std::string id = f.startedInstance();

    SUBCASE("start before assign") {
        CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);
        CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));   // still Available
    }
    SUBCASE("complete before assign") {
        CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error);
        CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));
    }
    SUBCASE("complete before start (skips Started)") {
        f.facade.assignTask(id, "1", "alice");
        CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error);
        CHECK_NOTHROW(f.facade.startTask(id, "1"));             // still Assigned
        CHECK_NOTHROW(f.facade.completeTask(id, "1"));
    }
    SUBCASE("assign twice") {
        f.facade.assignTask(id, "1", "alice");
        CHECK_THROWS_AS(f.facade.assignTask(id, "1", "bob"), std::logic_error);
        CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));     // first assignee kept
        CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "bob"));
    }
    SUBCASE("assign after start") {
        f.facade.assignTask(id, "1", "alice");
        f.facade.startTask(id, "1");
        CHECK_THROWS_AS(f.facade.assignTask(id, "1", "bob"), std::logic_error);
    }
    SUBCASE("start twice") {
        f.facade.assignTask(id, "1", "alice");
        f.facade.startTask(id, "1");
        CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);
    }
}

TEST_CASE("a Completed task is final: nothing can be done to it") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    f.facade.completeTask(id, "1");
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "bob"), std::logic_error);
    CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);
    CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error);
    CHECK_THROWS_AS(f.facade.rejectTask(id, "1", "too late"), std::logic_error);
}

TEST_CASE("rejection is possible from Available, Assigned and Started") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 3));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);

    f.facade.rejectTask(id, "1", "from available");        // Available -> Rejected
    f.facade.assignTask(id, "2", "alice");
    f.facade.rejectTask(id, "2", "from assigned");         // Assigned -> Rejected
    f.facade.assignTask(id, "3", "alice");
    f.facade.startTask(id, "3");
    f.facade.rejectTask(id, "3", "from started");          // Started -> Rejected

    std::vector<std::string> log = f.facade.getAuditLog();
    CHECK(countContaining(log, "(now Rejected)") == 3);
}

TEST_CASE("rejection is impossible while Created (instance not started)") {
    Fixture f;
    std::string id = f.newInstance();
    CHECK_THROWS_AS(f.facade.rejectTask(id, "1", "too early"), std::logic_error);
}

TEST_CASE("a Rejected task is a dead end for assign / start / complete / reject") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.rejectTask(id, "1", "bad input");
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "alice"), std::logic_error);
    CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);
    CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error);
    CHECK_THROWS_AS(f.facade.rejectTask(id, "1", "again"), std::logic_error);
}

TEST_CASE("unusual rejection reasons are accepted verbatim") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 3));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);

    const std::string multiline = "line one\nline two\ttabbed";
    const std::string longReason(5000, 'x');
    f.facade.rejectTask(id, "1", "");                      // empty
    f.facade.rejectTask(id, "2", multiline);               // newline + tab
    f.facade.rejectTask(id, "3", longReason);              // very long

    std::vector<std::string> log = f.facade.getAuditLog();
    CHECK(countContaining(log, multiline) == 1);
    CHECK(countContaining(log, longReason) == 1);
}

TEST_CASE("an invalid transition emits no audit event") {
    Fixture f;
    std::string id = f.startedInstance();
    const std::size_t before = f.facade.getAuditLog().size();
    CHECK_THROWS(f.facade.completeTask(id, "1"));
    CHECK_THROWS(f.facade.startTask(id, "1"));
    CHECK(f.facade.getAuditLog().size() == before);
}

TEST_CASE("operations still need the instance to be started") {
    Fixture f;
    std::string id = f.newInstance();
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "alice"), std::logic_error);
    CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);
    CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error);
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Isolation (Prototype cloning hidden behind the facade)
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: instance isolation") {

TEST_CASE("progress in one instance does not leak into another") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    std::string a = f.facade.createInstance("seq");
    std::string b = f.facade.createInstance("seq");
    f.facade.startInstance(a);
    f.facade.startInstance(b);

    f.facade.assignTask(a, "1", "alice");
    f.facade.startTask(a, "1");
    f.facade.completeTask(a, "1");

    // B's Task1 is still Available and fully usable
    CHECK_FALSE(f.facade.isTaskAssignedTo(b, "1", "alice"));
    CHECK_THROWS_AS(f.facade.startTask(b, "1"), std::logic_error);     // not assigned yet
    CHECK_NOTHROW(f.facade.assignTask(b, "1", "bob"));
    CHECK(f.facade.isTaskAssignedTo(b, "1", "bob"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(a, "1", "bob"));
}

TEST_CASE("starting one instance does not start another") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    std::string a = f.facade.createInstance("seq");
    std::string b = f.facade.createInstance("seq");
    f.facade.startInstance(a);
    CHECK_THROWS_AS(f.facade.assignTask(b, "1", "alice"), std::logic_error);   // b still Created
    CHECK_NOTHROW(f.facade.startInstance(b));
}

TEST_CASE("rejecting a task in one instance does not reject the same task in another") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    std::string a = f.facade.createInstance("seq");
    std::string b = f.facade.createInstance("seq");
    f.facade.startInstance(a);
    f.facade.startInstance(b);
    f.facade.rejectTask(a, "1", "only in A");
    CHECK_NOTHROW(f.facade.assignTask(b, "1", "alice"));
    CHECK_THROWS_AS(f.facade.assignTask(a, "1", "alice"), std::logic_error);
}

TEST_CASE("instances do not mutate the registered definition (new instances start fresh)") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    std::string a = f.facade.createInstance("seq");
    f.facade.startInstance(a);
    f.facade.assignTask(a, "1", "alice");
    f.facade.startTask(a, "1");
    f.facade.completeTask(a, "1");

    std::string b = f.facade.createInstance("seq");           // after A has fully progressed
    CHECK_THROWS_AS(f.facade.assignTask(b, "1", "alice"), std::logic_error);  // fresh = Created
    f.facade.startInstance(b);
    CHECK_NOTHROW(f.facade.assignTask(b, "1", "alice"));      // fresh = Available
}

TEST_CASE("instances of different definitions are independent even with identical item IDs") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("hospital", &f.workerRole, 1));
    f.facade.registerDefinition(makeSequentialDefinition("insurance", &f.workerRole, 1));
    std::string h = f.facade.createInstance("hospital");
    std::string i = f.facade.createInstance("insurance");
    f.facade.startInstance(h);
    f.facade.startInstance(i);
    f.facade.assignTask(h, "1", "alice");
    CHECK(f.facade.isTaskAssignedTo(h, "1", "alice"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(i, "1", "alice"));
}

TEST_CASE("task data configured on the definition is copied into every instance independently") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("seq", &f.workerRole, 1);
    tagTasks(def, "k", "v");
    f.facade.registerDefinition(def);
    std::string a = f.facade.createInstance("seq");
    std::string b = f.facade.createInstance("seq");
    // mutate the definition's prototype after the fact (it is only frozen structurally);
    // existing instances must be unaffected, proving they hold deep copies
    def->getRoot()->getChildren()[0]->setData("k", "changed");
    f.facade.startInstance(a);
    f.facade.startInstance(b);
    CHECK_NOTHROW(f.facade.assignTask(a, "1", "alice"));
    CHECK_NOTHROW(f.facade.assignTask(b, "1", "alice"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Parallel stage behaviour through the facade
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: parallel work") {

TEST_CASE("parallel tasks can be worked in any order and by different people") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 3));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);

    f.facade.assignTask(id, "3", "alice");
    f.facade.assignTask(id, "1", "bob");
    f.facade.assignTask(id, "2", "alice");
    f.facade.startTask(id, "2");
    f.facade.startTask(id, "3");
    f.facade.completeTask(id, "3");
    f.facade.completeTask(id, "2");
    f.facade.startTask(id, "1");
    f.facade.completeTask(id, "1");

    CHECK(countContaining(f.facade.getAuditLog(), "was completed") == 3);
}

TEST_CASE("one parallel task failing does not block its siblings") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 2));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    f.facade.rejectTask(id, "1", "failed");
    f.facade.assignTask(id, "2", "alice");
    f.facade.startTask(id, "2");
    CHECK_NOTHROW(f.facade.completeTask(id, "2"));
}

TEST_CASE("nested stages: a parallel stage inside a sequential root") {
    Fixture f;
    DefinitionBuilder b;
    b.reset("nested", 1);
    b.beginSequentialStage("Root");     // id 0
    b.beginParallelStage("Par");        // id 1
    b.addTask("A", &f.workerRole, nullptr); b.withAudit(nullptr); b.commitPendingTask();  // id 2
    b.addTask("B", &f.workerRole, nullptr); b.withAudit(nullptr); b.commitPendingTask();  // id 3
    b.endStage();
    b.endStage();
    f.facade.registerDefinition(b.getResult());

    std::string id = f.facade.createInstance("nested");
    f.facade.startInstance(id);
    CHECK_NOTHROW(f.facade.assignTask(id, "2", "alice"));
    CHECK_NOTHROW(f.facade.assignTask(id, "3", "bob"));
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "bob"), std::logic_error);   // item 1 is a Stage
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Decorators wrapped around tasks must be transparent to the facade
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: decorated tasks") {

TEST_CASE("a ValidatedTask blocks completion until its data is present (runtime_error)") {
    Fixture f;
    DefinitionBuilder b;
    b.reset("validated", 1);
    b.beginSequentialStage("S");
    b.addTask("Check", &f.workerRole, nullptr);
    b.withValidation(std::vector<std::string>(1, "formID"));
    b.withAudit(nullptr);
    b.commitPendingTask();
    b.endStage();
    WorkflowDefinition* def = b.getResult();
    f.facade.registerDefinition(def);

    std::string id = f.facade.createInstance("validated");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::runtime_error);
    CHECK(contains(errorOf([&] { f.facade.completeTask(id, "1"); }), "formID"));
}

TEST_CASE("a failed completion (validation) leaves the task Started and not logged as completed") {
    Fixture f;
    DefinitionBuilder b;
    b.reset("validated", 1);
    b.beginSequentialStage("S");
    b.addTask("Check", &f.workerRole, nullptr);
    b.withValidation(std::vector<std::string>(1, "formID"));
    b.withAudit(nullptr);
    b.commitPendingTask();
    b.endStage();
    f.facade.registerDefinition(b.getResult());
    std::string id = f.facade.createInstance("validated");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    CHECK_THROWS(f.facade.completeTask(id, "1"));
    CHECK(countContaining(f.facade.getAuditLog(), "was completed") == 0);
    CHECK(f.facade.undoLastAction());   // the earlier start is still undoable
}

TEST_CASE("a PriorityTask behaves like a normal task through the facade") {
    Fixture f;
    DefinitionBuilder b;
    b.reset("prio", 1);
    b.beginSequentialStage("S");
    b.addTask("Urgent", &f.workerRole, nullptr);
    b.withPriority(9);
    b.withAudit(nullptr);
    b.commitPendingTask();
    b.endStage();
    f.facade.registerDefinition(b.getResult());
    std::string id = f.facade.createInstance("prio");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    CHECK_NOTHROW(f.facade.completeTask(id, "1"));
}

} // TEST_SUITE

// ---------------------------------------------------------------------------
// Lifetime / ownership
// ---------------------------------------------------------------------------
TEST_SUITE("Facade: lifetime and ownership") {

TEST_CASE("an empty facade can be created and destroyed") {
    TIVIDYFacade* f = new TIVIDYFacade();
    delete f;
    CHECK(true);
}

TEST_CASE("facade destroys cleanly with instances in every lifecycle state") {
    Role role("r", 1);
    {
        TIVIDYFacade facade;
        facade.registerParticipant(new Participant("p", "P", &role));
        facade.registerDefinition(makeParallelDefinition("par", &role, 5));
        std::string id = facade.createInstance("par");
        facade.startInstance(id);
        facade.assignTask(id, "1", "p");                                   // Assigned
        facade.assignTask(id, "2", "p"); facade.startTask(id, "2");        // Started
        facade.assignTask(id, "3", "p"); facade.startTask(id, "3");
        facade.completeTask(id, "3");                                      // Completed
        facade.rejectTask(id, "4", "no");                                  // Rejected
        // task 5 left Available; one more instance never started (Created)
        facade.createInstance("par");
    }   // ASan/leak sanitiser would flag any leak or double free here
    CHECK(true);
}

TEST_CASE("facade with approval chain and many commands destroys cleanly") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("appr", &f.workerRole, 1);
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("appr");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    f.facade.requestApproval(id, "1");
    CHECK(true);
}

TEST_CASE("the audit log and metrics getters return values, not live references") {
    Fixture f;
    f.startedInstance();
    std::vector<std::string> copy = f.facade.getAuditLog();
    const std::size_t n = copy.size();
    copy.push_back("tampered");
    copy.clear();
    CHECK(f.facade.getAuditLog().size() == n);   // internal log unaffected
}

} // TEST_SUITE