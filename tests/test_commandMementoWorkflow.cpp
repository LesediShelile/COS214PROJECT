// Comprehensive regression tests for Command, Memento, WorkflowDefinition,
// and WorkflowInstance. Uses the doctest main supplied by test_work_items.cpp.
#include "third_party/doctest.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AssignCommand.h"
#include "CommandHistory.h"
#include "CommandInvoker.h"
#include "CompleteCommand.h"
#include "ParallelStage.h"
#include "Participant.h"
#include "RejectCommand.h"
#include "Role.h"
#include "SequentialStage.h"
#include "StartCommand.h"
#include "Task.h"
#include "TaskMemento.h"
#include "WorkflowCommand.h"
#include "WorkflowDefinition.h"
#include "WorkflowInstance.h"

namespace {
Task* makeTask(const std::string& id, const std::string& name = "task") {
    return new Task(id, name, 0, 0);
}
}

TEST_CASE("WorkflowCommand rejects null targets") {
    CHECK_THROWS_AS(StartCommand(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(CompleteCommand(nullptr), std::invalid_argument);
    CHECK_THROWS_AS(RejectCommand(nullptr, "reason"), std::invalid_argument);
}

TEST_CASE("AssignCommand rejects null participants and descriptions are stable") {
    std::unique_ptr<Task> task(makeTask("t1", "Review"));
    CHECK_THROWS_AS(AssignCommand(task.get(), nullptr), std::invalid_argument);
    Role role("Reviewer", 1);
    Participant p("p1", "Alex", &role);
    AssignCommand command(task.get(), &p);
    CHECK(command.getDescription() == "Assign 'Review' to Alex");
}

TEST_CASE("Command descriptions preserve names and rejection reasons") {
    std::unique_ptr<Task> task(makeTask("t1", "Review"));
    StartCommand start(task.get());
    CompleteCommand complete(task.get());
    RejectCommand reject(task.get(), "missing evidence");
    CHECK(start.getDescription() == "Start 'Review'");
    CHECK(complete.getDescription() == "Complete 'Review'");
    CHECK(reject.getDescription() == "Reject 'Review': missing evidence");
    RejectCommand emptyReason(task.get(), "");
    CHECK(emptyReason.getDescription() == "Reject 'Review': ");
}

TEST_CASE("Undo before execute fails safely") {
    std::unique_ptr<Task> task(makeTask("t1"));
    StartCommand command(task.get());
    CHECK_THROWS_AS(command.undo(), std::logic_error);
    CHECK(task->getStateName() == "Created");
}

TEST_CASE("StartCommand execute and undo restore exact prior state") {
    std::unique_ptr<Task> task(makeTask("t1"));
    task->makeAvailable();
    Role role("Worker", 1);
    Participant p("p1", "Sam", &role);
    task->assign(&p);
    task->setData("key", "before");
    StartCommand command(task.get());
    command.execute();
    CHECK(task->getStateName() == "Started");
    command.undo();
    CHECK(task->getStateName() == "Assigned");
    CHECK(task->getAssignee() == &p);
    CHECK(task->getData("key") == "before");
}

TEST_CASE("AssignCommand undo restores the available state and clears the new assignee") {
    std::unique_ptr<Task> task(makeTask("t1"));
    task->makeAvailable();
    Role role("Worker", 1);
    Participant second("p2", "Second", &role);
    task->setData("case", "original");

    AssignCommand command(task.get(), &second);
    command.execute();
    CHECK(task->getAssignee() == &second);
    command.undo();
    CHECK(task->getAssignee() == nullptr);
    CHECK(task->getStateName() == "Available");
    CHECK(task->getData("case") == "original");
}

TEST_CASE("CompleteCommand can undo completion to Started state") {
    std::unique_ptr<Task> task(makeTask("t1"));
    task->makeAvailable();
    Role role("Worker", 1);
    Participant p("p1", "Sam", &role);
    task->assign(&p);
    task->start();
    CompleteCommand command(task.get());
    command.execute();
    CHECK(task->isFinished());
    CHECK(task->getStateName() == "Completed");
    command.undo();
    CHECK_FALSE(task->isFinished());
    CHECK(task->getStateName() == "Started");
    CHECK(task->getAssignee() == &p);
}

TEST_CASE("RejectCommand can undo rejection and restore previous state") {
    std::unique_ptr<Task> task(makeTask("t1"));
    task->makeAvailable();
    Role role("Worker", 1);
    Participant p("p1", "Sam", &role);
    task->assign(&p);
    task->start();
    RejectCommand command(task.get(), "incorrect");
    command.execute();
    CHECK(task->getStateName() == "Rejected");
    command.undo();
    CHECK(task->getStateName() == "Started");
    CHECK(task->getAssignee() == &p);
}

TEST_CASE("Memento restore is repeatable and snapshots do not change with later mutations") {
    std::unique_ptr<Task> task(makeTask("t1"));
    Role role("Worker", 1);
    Participant p("p1", "Sam", &role);
    task->makeAvailable();
    task->assign(&p);
    task->setData("field", "snapshot");
    std::unique_ptr<TaskMemento> snapshot(task->createMemento());

    task->start();
    task->setData("field", "mutated");
    task->setData("new", "later");
    task->restoreMemento(snapshot.get());
    CHECK(task->getStateName() == "Assigned");
    CHECK(task->getAssignee() == &p);
    CHECK(task->getData("field") == "snapshot");
    CHECK_FALSE(task->hasData("new"));

    task->start();
    task->restoreMemento(snapshot.get());
    CHECK(task->getStateName() == "Assigned");
    CHECK(task->getData("field") == "snapshot");
}

TEST_CASE("Memento rejects null restore") {
    std::unique_ptr<Task> task(makeTask("t1"));
    CHECK_THROWS_AS(task->restoreMemento(nullptr), std::invalid_argument);
}

TEST_CASE("CommandHistory handles empty, null, LIFO, and repeated operations") {
    CommandHistory history;
    CHECK(history.isEmpty());
    CHECK(history.size() == 0);
    CHECK(history.pop() == nullptr);
    CHECK_THROWS_AS(history.push(nullptr), std::invalid_argument);

    std::unique_ptr<Task> task(makeTask("t1"));
    WorkflowCommand* first = new StartCommand(task.get());
    WorkflowCommand* second = new CompleteCommand(task.get());
    history.push(first);
    history.push(second);
    CHECK(history.size() == 2);
    CHECK_FALSE(history.isEmpty());
    CHECK(history.pop() == second);
    delete second;
    CHECK(history.size() == 1);
    CHECK(history.pop() == first);
    delete first;
    CHECK(history.isEmpty());
    CHECK(history.pop() == nullptr);
}

TEST_CASE("CommandInvoker empty undo and null command are safe") {
    CommandInvoker invoker;
    CHECK_FALSE(invoker.canUndo());
    CHECK_FALSE(invoker.undoLast());
    CHECK_THROWS_AS(invoker.executeCommand(nullptr), std::invalid_argument);
    CHECK(invoker.getHistory()->size() == 0);
}

TEST_CASE("CommandInvoker records successful commands and undoes in reverse order") {
    std::unique_ptr<Task> task(makeTask("t1"));
    CommandInvoker invoker;
    task->makeAvailable();
    Role role("Worker", 1);
    Participant participant("p1", "Sam", &role);
    task->assign(&participant);
    invoker.executeCommand(new StartCommand(task.get()));
    CHECK(task->getStateName() == "Started");
    CHECK(invoker.canUndo());
    CHECK(invoker.getHistory()->size() == 1);
    CHECK(invoker.undoLast());
    CHECK(task->getStateName() == "Assigned");
    CHECK_FALSE(invoker.canUndo());
    CHECK_FALSE(invoker.undoLast());
}

TEST_CASE("CommandInvoker does not retain a command whose execution throws") {
    std::unique_ptr<Task> task(makeTask("t1"));
    CommandInvoker invoker;
    CHECK_THROWS_AS(invoker.executeCommand(new StartCommand(task.get())), std::logic_error);
    CHECK_FALSE(invoker.canUndo());
    CHECK(invoker.getHistory()->size() == 0);
    CHECK(task->getStateName() == "Created");
}

TEST_CASE("WorkflowDefinition exposes metadata and enforces freeze rules") {
    WorkflowDefinition definition("admission", 3);
    CHECK(definition.getName() == "admission");
    CHECK(definition.getVersion() == 3);
    CHECK_FALSE(definition.isFrozen());
    CHECK(definition.getRoot() == nullptr);
    CHECK_THROWS_AS(definition.freeze(), std::logic_error);
    CHECK_THROWS_AS(definition.createInstance("i1"), std::logic_error);
    CHECK_THROWS_AS(definition.setRoot(nullptr), std::invalid_argument);

    definition.setRoot(makeTask("root", "Root"));
    CHECK(definition.getRoot() != nullptr);
    definition.freeze();
    CHECK(definition.isFrozen());
    CHECK_NOTHROW(definition.freeze());
    WorkItem* replacement = makeTask("replacement");
    CHECK_THROWS_AS(definition.setRoot(replacement), std::logic_error);
    delete replacement; // rejected by setRoot, so ownership was never transferred
    CHECK_THROWS_AS(definition.setApprovalChain(nullptr), std::logic_error);
}

TEST_CASE("WorkflowDefinition setRoot same pointer is a no-op before freeze") {
    WorkflowDefinition definition("wf", 1);
    Task* root = makeTask("root");
    definition.setRoot(root);
    CHECK_NOTHROW(definition.setRoot(root));
    CHECK(definition.getRoot() == root);
    definition.freeze();
}

TEST_CASE("WorkflowInstance rejects null and unfrozen definitions") {
    CHECK_THROWS_AS(WorkflowInstance("i1", nullptr), std::invalid_argument);
    WorkflowDefinition definition("wf", 1);
    definition.setRoot(makeTask("root"));
    CHECK_THROWS_AS(WorkflowInstance("i1", &definition), std::logic_error);
}

TEST_CASE("WorkflowInstance clones tree and isolates instances from definition and each other") {
    WorkflowDefinition definition("wf", 7);
    SequentialStage* root = new SequentialStage("root", "Root");
    root->add(makeTask("a", "A"));
    root->add(makeTask("b", "B"));
    definition.setRoot(root);
    definition.freeze();

    std::unique_ptr<WorkflowInstance> one(definition.createInstance("one"));
    std::unique_ptr<WorkflowInstance> two(definition.createInstance("two"));
    CHECK(one->getInstanceID() == "one");
    CHECK(one->getDefinition() == &definition);
    CHECK(one->getDefinitionVersion() == 7);
    CHECK(one->getRoot() != definition.getRoot());
    CHECK(one->getRoot() != two->getRoot());
    CHECK(one->getStatus() == WorkflowStatus::CREATED);
    CHECK_FALSE(one->isFinished());
    CHECK(one->getPublisher() != nullptr);
    CHECK(one->getPublisher() != two->getPublisher());

    one->start();
    CHECK(one->getStatus() == WorkflowStatus::RUNNING);
    CHECK(two->getStatus() == WorkflowStatus::CREATED);
    CHECK(one->getAvailableItems().size() == 1);
    CHECK(two->getAvailableItems().empty());
    CHECK(definition.getRoot()->getStateName() == "Pending");
}

TEST_CASE("WorkflowInstance start twice throws and leaves running instance unchanged") {
    WorkflowDefinition definition("wf", 1);
    definition.setRoot(makeTask("root"));
    definition.freeze();
    std::unique_ptr<WorkflowInstance> instance(definition.createInstance("i1"));
    instance->start();
    CHECK_THROWS_AS(instance->start(), std::logic_error);
    CHECK(instance->getStatus() == WorkflowStatus::RUNNING);
}

TEST_CASE("WorkflowInstance sequential end-to-end execution completes") {
    WorkflowDefinition definition("wf", 1);
    SequentialStage* root = new SequentialStage("root", "Root");
    root->add(makeTask("a", "A"));
    root->add(makeTask("b", "B"));
    definition.setRoot(root);
    definition.freeze();
    std::unique_ptr<WorkflowInstance> instance(definition.createInstance("i1"));
    Role role("Worker", 1);
    Participant p("p1", "Sam", &role);

    instance->start();
    WorkItem* rootItem = instance->getRoot();
    std::vector<WorkItem*> children = rootItem->getChildren();
    REQUIRE(children.size() == 2);
    children[0]->assign(&p);
    children[0]->start();
    children[0]->complete();
    instance->activateNext();
    children[1]->assign(&p);
    children[1]->start();
    children[1]->complete();
    instance->activateNext();

    CHECK(instance->getStatus() == WorkflowStatus::COMPLETED);
    CHECK(instance->isFinished());
    CHECK(instance->getOutcome() == "Completed");
    CHECK_THROWS_AS(instance->cancel(), std::logic_error);
    CHECK_THROWS_AS(instance->fail("too late"), std::logic_error);
}

TEST_CASE("WorkflowInstance cancel from CREATED and RUNNING are terminal") {
    WorkflowDefinition definition("wf", 1);
    SequentialStage* root = new SequentialStage("root", "Root");
    root->add(makeTask("a", "A"));
    definition.setRoot(root);
    definition.freeze();

    std::unique_ptr<WorkflowInstance> beforeStart(definition.createInstance("before"));
    beforeStart->cancel();
    CHECK(beforeStart->getStatus() == WorkflowStatus::CANCELLED);
    CHECK(beforeStart->isFinished());
    CHECK(beforeStart->getOutcome() == "Cancelled");
    CHECK_THROWS_AS(beforeStart->cancel(), std::logic_error);

    std::unique_ptr<WorkflowInstance> running(definition.createInstance("running"));
    running->start();
    running->cancel();
    CHECK(running->getStatus() == WorkflowStatus::CANCELLED);
    CHECK(running->isFinished());
}

TEST_CASE("WorkflowInstance failure records reason and rejects repeated terminal actions") {
    WorkflowDefinition definition("wf", 1);
    definition.setRoot(makeTask("root"));
    definition.freeze();
    std::unique_ptr<WorkflowInstance> instance(definition.createInstance("i1"));

    instance->fail("external service unavailable");
    CHECK(instance->getStatus() == WorkflowStatus::FAILED);
    CHECK(instance->isFinished());
    CHECK(instance->getOutcome() == "external service unavailable");
    CHECK_THROWS_AS(instance->fail("again"), std::logic_error);
    CHECK_THROWS_AS(instance->cancel(), std::logic_error);
}

TEST_CASE("WorkflowInstance outcome setter and empty failure reason fallback") {
    WorkflowDefinition definition("wf", 1);
    definition.setRoot(makeTask("root"));
    definition.freeze();
    std::unique_ptr<WorkflowInstance> instance(definition.createInstance("i1"));
    instance->setOutcome("custom");
    CHECK(instance->getOutcome() == "custom");
    instance->fail("");
    CHECK(instance->getOutcome() == "custom");
}

TEST_CASE("WorkflowInstance activateNext is harmless outside RUNNING") {
    WorkflowDefinition definition("wf", 1);
    definition.setRoot(makeTask("root"));
    definition.freeze();
    std::unique_ptr<WorkflowInstance> instance(definition.createInstance("i1"));
    CHECK_NOTHROW(instance->activateNext());
    CHECK(instance->getStatus() == WorkflowStatus::CREATED);
    instance->cancel();
    CHECK_NOTHROW(instance->activateNext());
    CHECK(instance->getStatus() == WorkflowStatus::CANCELLED);
}

TEST_CASE("Empty workflow stage completes on activation") {
    WorkflowDefinition definition("empty", 1);
    definition.setRoot(new SequentialStage("root", "Empty"));
    definition.freeze();
    std::unique_ptr<WorkflowInstance> instance(definition.createInstance("i1"));
    instance->start();
    instance->activateNext();
    CHECK(instance->getStatus() == WorkflowStatus::COMPLETED);
    CHECK(instance->getOutcome() == "Completed");
}
