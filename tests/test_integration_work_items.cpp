// Integration tests: Composite + Prototype + Decorator working together with
// the team's State, Command/Memento, Observer and Iterator code.
#include "doctest.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AssignCommand.h"
#include "AuditedTask.h"
#include "CommandInvoker.h"
#include "CompleteCommand.h"
#include "EventPublisher.h"
#include "ParallelStage.h"
#include "Participant.h"
#include "PriorityTask.h"
#include "Role.h"
#include "SequentialStage.h"
#include "StartCommand.h"
#include "Task.h"
#include "ValidatedTask.h"
#include "WorkItemIterator.h"
#include "WorkflowEvent.h"
#include "WorkflowListener.h"

namespace {

class Counter : public WorkflowListener {
public:
    Counter() : count(0) {}
    int count;
    virtual void update(WorkflowEvent*) { ++count; }
};

/** Task wrapped as Audited( Validated( Priority( Task ) ) ). */
WorkItem* decorated(const std::string& id, EventPublisher* pub, const std::string& requiredField) {
    std::vector<std::string> req;
    if (!requiredField.empty()) req.push_back(requiredField);
    return new AuditedTask(
        new ValidatedTask(new PriorityTask(new Task(id, "task " + id, 0, 0), 5), req), pub);
}

std::vector<std::string> availableIds(WorkItem& root) {
    std::vector<std::string> ids;
    std::unique_ptr<WorkItemIterator> it(root.createAvailableIterator());
    for (it->first(); !it->isDone(); it->next()) ids.push_back(it->current()->getItemID());
    return ids;
}

} // namespace

TEST_CASE("Commands run on a stacked decorator and undo restores each state") {
    EventPublisher pub;
    Counter counter;
    pub.attach(&counter);
    Role role("Nurse", 1);
    Participant p("p1", "Sam", &role);
    std::unique_ptr<WorkItem> t(decorated("t1", &pub, ""));
    t->makeAvailable();

    CommandInvoker inv;
    inv.executeCommand(new AssignCommand(t.get(), &p));
    inv.executeCommand(new StartCommand(t.get()));
    inv.executeCommand(new CompleteCommand(t.get()));
    CHECK(t->getStateName() == "Completed");
    CHECK(t->getData("priority") == "5");

    CHECK(inv.undoLast());
    CHECK(t->getStateName() == "Started");
    CHECK(inv.undoLast());
    CHECK(t->getStateName() == "Assigned");
    CHECK(inv.undoLast());
    CHECK(t->getStateName() == "Available");
    CHECK_FALSE(inv.canUndo());

    // makeAvailable + the three commands each published one audit event
    CHECK(counter.count == 4);
}

TEST_CASE("A command that hits ValidatedTask validation fails and is not recorded for undo") {
    Role role("Doctor", 2);
    Participant p("p1", "Dr", &role);
    std::unique_ptr<WorkItem> t(decorated("t1", NULL, "diagnosis"));
    t->makeAvailable();

    CommandInvoker inv;
    inv.executeCommand(new AssignCommand(t.get(), &p));
    inv.executeCommand(new StartCommand(t.get()));
    CHECK_THROWS(inv.executeCommand(new CompleteCommand(t.get())));
    CHECK(t->getStateName() == "Started");

    // only assign and start were recorded
    CHECK(inv.undoLast());
    CHECK(inv.undoLast());
    CHECK_FALSE(inv.canUndo());

    t->setData("diagnosis", "flu");
    inv.executeCommand(new AssignCommand(t.get(), &p));
    inv.executeCommand(new StartCommand(t.get()));
    CHECK_NOTHROW(inv.executeCommand(new CompleteCommand(t.get())));
    CHECK(t->isFinished());
}

TEST_CASE("A sequential stage of decorated tasks offers one task at a time") {
    EventPublisher pub;
    Role role("Staff", 1);
    Participant p("p1", "Pat", &role);
    SequentialStage root("root", "admission");
    root.add(decorated("a", &pub, ""));
    root.add(decorated("b", &pub, ""));
    root.add(decorated("c", &pub, ""));

    root.makeAvailable();
    CHECK(availableIds(root) == std::vector<std::string>(1, "a"));

    CommandInvoker inv;
    std::vector<WorkItem*> kids = root.getChildren();
    for (std::size_t i = 0; i < kids.size(); ++i) {
        inv.executeCommand(new AssignCommand(kids[i], &p));
        inv.executeCommand(new StartCommand(kids[i]));
        inv.executeCommand(new CompleteCommand(kids[i]));
        root.activateChildren();
        if (i + 1 < kids.size())
            CHECK(availableIds(root) == std::vector<std::string>(1, kids[i + 1]->getItemID()));
    }
    CHECK(root.isFinished());
    CHECK(availableIds(root).empty());
}

TEST_CASE("A parallel stage offers all its decorated tasks at once") {
    ParallelStage root("root", "prep");
    root.add(decorated("a", NULL, ""));
    root.add(decorated("b", NULL, ""));
    root.makeAvailable();
    CHECK(availableIds(root).size() == 2);
}

TEST_CASE("A cloned stage tree of decorated tasks runs independently of the original") {
    EventPublisher pub;
    Counter counter;
    pub.attach(&counter);
    Role role("Staff", 1);
    Participant p("p1", "Pat", &role);

    SequentialStage* original = new SequentialStage("root", "template");
    original->add(decorated("a", &pub, ""));
    original->add(decorated("b", &pub, ""));
    std::unique_ptr<WorkItem> copy(original->clone());

    copy->makeAvailable();
    CommandInvoker inv;
    WorkItem* first = copy->getChildren()[0];
    inv.executeCommand(new AssignCommand(first, &p));
    inv.executeCommand(new StartCommand(first));
    inv.executeCommand(new CompleteCommand(first));
    CHECK(first->isFinished());
    CHECK(first->getData("priority") == "5");   // decorator survived the clone

    // the template is untouched, and still behaves like a fresh workflow
    CHECK_FALSE(original->isFinished());
    CHECK(original->getChildren()[0]->getStateName() == "Created");
    original->makeAvailable();
    CHECK(availableIds(*original) == std::vector<std::string>(1, "a"));
    delete original;
}
