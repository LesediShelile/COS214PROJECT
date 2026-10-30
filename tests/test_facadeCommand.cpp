/**
 * @file test_facade_command_undo.cpp
 * @brief COMMAND (+ Memento) pattern behaviour reached through TIVIDYFacade.
 *
 * GoF mapping:
 *  - Invoker  : CommandInvoker (owned by the facade)
 *  - Commands : AssignCommand, StartCommand, CompleteCommand, RejectCommand
 *  - Receiver : WorkItem (Task)
 *  - Memento  : TaskMemento (snapshot taken in execute(), restored in undo())
 *
 * The undo stack is facade-wide (shared by all instances).
 */
#include "third_party/doctest.h"
#include "third_party/TestSupport.h"

using namespace testsupport;

TEST_SUITE("Facade/Command: undo basics") {

TEST_CASE("undo with an empty history returns false, repeatedly") {
    Fixture f;
    CHECK_FALSE(f.facade.undoLastAction());
    CHECK_FALSE(f.facade.undoLastAction());
}

TEST_CASE("undo on a brand-new facade (no participants/definitions) is safe") {
    TIVIDYFacade facade;
    CHECK_FALSE(facade.undoLastAction());
}

TEST_CASE("startInstance and createInstance are not undoable commands") {
    Fixture f;
    f.startedInstance();
    CHECK_FALSE(f.facade.undoLastAction());
}

TEST_CASE("undo reverts an assignment (state AND assignee)") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    REQUIRE(f.facade.isTaskAssignedTo(id, "1", "alice"));

    CHECK(f.facade.undoLastAction());
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);   // back to Available
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "bob"));                // and re-assignable
    CHECK(f.facade.isTaskAssignedTo(id, "1", "bob"));
}

TEST_CASE("undo reverts a start (Started -> Assigned)") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    CHECK(f.facade.undoLastAction());
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));               // still assigned
    CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error); // not started any more
    CHECK_NOTHROW(f.facade.startTask(id, "1"));                        // can start again
}

TEST_CASE("undo reverts a completion (Completed -> Started) and it can be completed again") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    f.facade.completeTask(id, "1");
    CHECK_THROWS_AS(f.facade.startTask(id, "1"), std::logic_error);    // final state
    CHECK(f.facade.undoLastAction());
    CHECK_NOTHROW(f.facade.completeTask(id, "1"));
}

TEST_CASE("undo reverts a rejection and restores the assignee") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.startTask(id, "1");
    f.facade.rejectTask(id, "1", "oops");
    REQUIRE_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));        // reject cleared it
    CHECK(f.facade.undoLastAction());
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));                // memento restored it
    CHECK_NOTHROW(f.facade.completeTask(id, "1"));                     // Started again
}

TEST_CASE("undo reverts a rejection from the Available state") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.rejectTask(id, "1", "dup");
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "alice"), std::logic_error);   // Rejected
    CHECK(f.facade.undoLastAction());
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));                       // Available again
}

} // TEST_SUITE

TEST_SUITE("Facade/Command: history semantics") {

TEST_CASE("undo is strictly LIFO and multi-level") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");   // cmd 1
    f.facade.startTask(id, "1");             // cmd 2
    f.facade.completeTask(id, "1");          // cmd 3

    CHECK(f.facade.undoLastAction());        // undo complete -> Started
    CHECK(f.facade.undoLastAction());        // undo start    -> Assigned
    CHECK(f.facade.undoLastAction());        // undo assign   -> Available
    CHECK_FALSE(f.facade.undoLastAction());  // exhausted
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

TEST_CASE("there is no redo: a second undo does not re-apply anything") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.undoLastAction();
    CHECK_FALSE(f.facade.undoLastAction());
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

TEST_CASE("a command that throws is not recorded, so undo targets the last SUCCESSFUL one") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");                              // success
    CHECK_THROWS_AS(f.facade.completeTask(id, "1"), std::logic_error);  // fails (Assigned)
    CHECK_THROWS_AS(f.facade.assignTask(id, "1", "bob"), std::logic_error);

    CHECK(f.facade.undoLastAction());                                   // undoes the ASSIGN
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK_FALSE(f.facade.undoLastAction());                             // nothing else recorded
}

TEST_CASE("failed lookups never create commands") {
    Fixture f;
    std::string id = f.startedInstance();
    CHECK_THROWS(f.facade.assignTask(id, "999", "alice"));
    CHECK_THROWS(f.facade.assignTask(id, "1", "ghost"));
    CHECK_THROWS(f.facade.assignTask("nope", "1", "alice"));
    CHECK_THROWS(f.facade.startTask("nope", "1"));
    CHECK_FALSE(f.facade.undoLastAction());
}

TEST_CASE("the undo stack is shared across instances: the most recent action wins") {
    Fixture f;
    f.facade.registerDefinition(makeSequentialDefinition("seq", &f.workerRole, 1));
    std::string a = f.facade.createInstance("seq");
    std::string b = f.facade.createInstance("seq");
    f.facade.startInstance(a);
    f.facade.startInstance(b);
    f.facade.assignTask(a, "1", "alice");
    f.facade.assignTask(b, "1", "bob");

    CHECK(f.facade.undoLastAction());                      // reverts B's assignment
    CHECK_FALSE(f.facade.isTaskAssignedTo(b, "1", "bob"));
    CHECK(f.facade.isTaskAssignedTo(a, "1", "alice"));     // A untouched
    CHECK(f.facade.undoLastAction());                      // now A's
    CHECK_FALSE(f.facade.isTaskAssignedTo(a, "1", "alice"));
}

TEST_CASE("undo then new action then undo: history is rebuilt correctly") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    f.facade.undoLastAction();
    f.facade.assignTask(id, "1", "bob");
    CHECK(f.facade.isTaskAssignedTo(id, "1", "bob"));
    CHECK(f.facade.undoLastAction());
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "bob"));
    CHECK_FALSE(f.facade.undoLastAction());
}

TEST_CASE("undo/redo-style cycling 50 times stays consistent") {
    Fixture f;
    std::string id = f.startedInstance();
    for (int i = 0; i < 50; ++i) {
        f.facade.assignTask(id, "1", "alice");
        REQUIRE(f.facade.isTaskAssignedTo(id, "1", "alice"));
        REQUIRE(f.facade.undoLastAction());
        REQUIRE_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
    }
    CHECK_FALSE(f.facade.undoLastAction());
}

TEST_CASE("the audit log is append-only: undo does not erase past events") {
    Fixture f;
    std::string id = f.startedInstance();
    f.facade.assignTask(id, "1", "alice");
    const std::size_t before = f.facade.getAuditLog().size();
    f.facade.undoLastAction();
    CHECK(f.facade.getAuditLog().size() >= before);
    CHECK(countContaining(f.facade.getAuditLog(), "was assigned") == 1);
}

TEST_CASE("undo works on decorated (audited + validated + priority) tasks") {
    Fixture f;
    DefinitionBuilder b;
    b.reset("deco", 1);
    b.beginSequentialStage("S");
    b.addTask("T", &f.workerRole, nullptr);
    b.withPriority(3);
    b.withValidation(std::vector<std::string>());
    b.withAudit(nullptr);
    b.commitPendingTask();
    b.endStage();
    f.facade.registerDefinition(b.getResult());
    std::string id = f.facade.createInstance("deco");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    CHECK(f.facade.undoLastAction());
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "1", "alice"));
}

TEST_CASE("a different task is not affected by undoing another task's command") {
    Fixture f;
    f.facade.registerDefinition(makeParallelDefinition("par", &f.workerRole, 2));
    std::string id = f.facade.createInstance("par");
    f.facade.startInstance(id);
    f.facade.assignTask(id, "1", "alice");
    f.facade.assignTask(id, "2", "bob");
    f.facade.undoLastAction();   // undo Task2's assignment
    CHECK(f.facade.isTaskAssignedTo(id, "1", "alice"));
    CHECK_FALSE(f.facade.isTaskAssignedTo(id, "2", "bob"));
}

} // TEST_SUITE