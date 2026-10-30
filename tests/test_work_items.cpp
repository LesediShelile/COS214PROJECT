
// Tests for Composite, Prototype and Iterator (work-item core).
// Build and run: see README.md (cmake --build build && ctest --test-dir build).

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AvailableItemsIterator.h"
#include "DepthFirstIterator.h"
#include "ParallelStage.h"
#include "SequentialStage.h"
#include "Stage.h"
#include "Task.h"
#include "TaskDecorator.h"
#include "Participant.h"
#include "Role.h"
#include "TaskMemento.h"
#include "TaskState.h"

namespace {

Task* newTask(const std::string& id) {
    return new Task(id, "task " + id, 0, 0);
}

/** Drive a task through its full happy path. */
void finish(WorkItem* t, Participant* p) {
    t->assign(p);
    t->start();
    t->complete();
}

/** Minimal concrete decorator used only to test the Decorator base. */
class TagDecorator : public TaskDecorator {
public:
    explicit TagDecorator(WorkItem* task) : TaskDecorator(task) {}
    TagDecorator(const TagDecorator& other) : TaskDecorator(other) {}

    virtual WorkItem* clone() const {
        return new TagDecorator(*this);
    }
};

std::vector<std::string> ids(WorkItemIterator* it) {
    std::vector<std::string> out;

    for (it->first(); !it->isDone(); it->next()) {
        out.push_back(it->current()->getItemID());
    }

    return out;
}

} // namespace

TEST_CASE("Task starts Created and follows its lifecycle through the state object") {
    std::unique_ptr<Task> t(newTask("t1"));

    Role role("Doctor", 2);
    Participant p("p1", "Dr Who", &role);

    CHECK(t->getStateName() == "CreatedState");
    CHECK_FALSE(t->isAvailable());

    t->makeAvailable();
    CHECK(t->isAvailable());

    t->assign(&p);
    CHECK(t->getAssignee() == &p);

    t->start();
    t->complete();

    CHECK(t->isFinished());
    CHECK(t->getStateName() == "CompletedState");
}

TEST_CASE("Invalid operations are rejected, not silently ignored") {
    std::unique_ptr<Task> t(newTask("t1"));

    CHECK_THROWS_AS(t->start(), std::logic_error);
    CHECK_THROWS_AS(t->complete(), std::logic_error);

    t->cancel();

    CHECK_THROWS_AS(t->makeAvailable(), std::logic_error);
}

TEST_CASE("Stage refuses unit-of-work operations and rejects null children") {
    SequentialStage s("s", "stage");

    Role role("General", 1);
    Participant p("p", "p", &role);

    CHECK_THROWS_AS(s.assign(&p), std::logic_error);
    CHECK_THROWS_AS(s.start(), std::logic_error);
    CHECK_THROWS_AS(s.complete(), std::logic_error);
    CHECK_THROWS_AS(s.reject("no"), std::logic_error);
    CHECK_THROWS_AS(s.add(0), std::invalid_argument);
}

TEST_CASE("Stage::isFinished aggregates over its children") {
    ParallelStage s("s", "stage");

    Task* a = newTask("a");
    Task* b = newTask("b");

    s.add(a);
    s.add(b);

    Role role("General", 1);
    Participant p("p", "p", &role);

    s.activateChildren();

    CHECK_FALSE(s.isFinished());

    finish(a, &p);
    CHECK_FALSE(s.isFinished());

    finish(b, &p);
    CHECK(s.isFinished());
    CHECK(s.getStateName() == "Finished");
}

TEST_CASE("SequentialStage releases children one at a time, in order") {
    SequentialStage s("s", "stage");

    Task* a = newTask("a");
    Task* b = newTask("b");
    Task* c = newTask("c");

    s.add(a);
    s.add(b);
    s.add(c);

    Role role("General", 1);
    Participant p("p", "p", &role);

    CHECK(s.getStateName() == "Pending");

    s.makeAvailable();

    CHECK(s.getStateName() == "InProgress");
    CHECK(a->isAvailable());
    CHECK_FALSE(b->isAvailable());

    finish(a, &p);
    s.activateChildren();

    CHECK(b->isAvailable());
    CHECK_FALSE(c->isAvailable());

    s.activateChildren(); // Calling again must not re-activate b.

    CHECK(b->getStateName() == "AvailableState");

    finish(b, &p);
    s.activateChildren();

    CHECK(c->isAvailable());

    finish(c, &p);
    s.activateChildren();

    CHECK(s.isFinished());
}

TEST_CASE("ParallelStage makes every child available at once") {
    ParallelStage s("s", "stage");

    Task* a = newTask("a");
    Task* b = newTask("b");

    s.add(a);
    s.add(b);

    s.makeAvailable();

    CHECK(a->isAvailable());
    CHECK(b->isAvailable());
}

TEST_CASE("Nested stages progress through the same uniform interface") {
    SequentialStage root("root", "root");

    ParallelStage* par = new ParallelStage("par", "par");

    Task* x = newTask("x");
    Task* y = newTask("y");
    Task* after = newTask("after");

    par->add(x);
    par->add(y);

    root.add(par);
    root.add(after);

    Role role("General", 1);
    Participant p("p", "p", &role);

    root.makeAvailable();

    CHECK(x->isAvailable());
    CHECK(y->isAvailable());
    CHECK_FALSE(after->isAvailable());

    finish(x, &p);
    root.activateChildren();

    CHECK_FALSE(after->isAvailable()); // y is still open.

    finish(y, &p);
    root.activateChildren();

    CHECK(after->isAvailable());
}

TEST_CASE("Stage::cancel cancels every unfinished child and leaves finished ones") {
    ParallelStage s("s", "stage");

    Task* a = newTask("a");
    Task* b = newTask("b");

    s.add(a);
    s.add(b);

    Role role("General", 1);
    Participant p("p", "p", &role);

    s.makeAvailable();

    finish(a, &p);
    s.cancel();

    CHECK(a->getStateName() == "CompletedState");
    CHECK(b->getStateName() == "CancelledState");
    CHECK(s.isFinished());
}

TEST_CASE("Prototype: a cloned tree is independent of the original") {
    SequentialStage* tmpl = new SequentialStage("root", "template");

    ParallelStage* par = new ParallelStage("par", "par");

    par->add(newTask("a"));
    par->add(newTask("b"));

    tmpl->add(par);
    tmpl->add(newTask("c"));

    tmpl->setData("patient", "template");

    std::unique_ptr<WorkItem> templ(tmpl);
    std::unique_ptr<WorkItem> copy1(templ->clone());
    std::unique_ptr<WorkItem> copy2(templ->clone());

    // Different objects all the way down.
    CHECK(copy1.get() != templ.get());
    CHECK(copy1->getChildren()[0] != templ->getChildren()[0]);

    CHECK(
        copy1->getChildren()[0]->getChildren()[0] !=
        templ->getChildren()[0]->getChildren()[0]
    );

    // Progressing copy1 leaves the template and copy2 untouched.
    Role role("General", 1);
    Participant p("p", "p", &role);

    copy1->makeAvailable();

    finish(copy1->getChildren()[0]->getChildren()[0], &p);
    copy1->setData("patient", "case 1");

    CHECK(copy1->getChildren()[0]->getChildren()[0]->isFinished());
    CHECK_FALSE(templ->getChildren()[0]->getChildren()[0]->isFinished());
    CHECK_FALSE(copy2->getChildren()[0]->getChildren()[0]->isFinished());

    CHECK(templ->getData("patient") == "template");
    CHECK(copy2->getData("patient") == "template");
    CHECK(copy1->getStateName() == "InProgress");
    CHECK(templ->getStateName() == "Pending");
}

TEST_CASE("Prototype: cloning a Task copies its state object, not the pointer") {
    std::unique_ptr<Task> t(newTask("t"));

    t->makeAvailable();

    std::unique_ptr<WorkItem> c(t->clone());

    CHECK(c->getStateName() == "AvailableState");

    Role role("General", 1);
    Participant p("p", "p", &role);

    c->assign(&p); // Moves only the clone.

    CHECK(c->getStateName() == "AssignedState");
    CHECK(t->getStateName() == "AvailableState");
}

TEST_CASE("Decorator forwards to the wrapped task and clones it deeply") {
    Task* inner = newTask("t");

    std::unique_ptr<WorkItem> dec(new TagDecorator(inner));

    CHECK(dec->getItemID() == "t");
    CHECK(dec->getName() == "task t");

    dec->setData("k", "v");

    CHECK(inner->getData("k") == "v");

    dec->makeAvailable();

    CHECK(dec->isAvailable());

    std::unique_ptr<WorkItem> copy(dec->clone());

    CHECK(static_cast<TaskDecorator*>(copy.get())->getWrapped() != inner);

    Role role("General", 1);
    Participant p("p", "p", &role);

    copy->assign(&p);

    CHECK(copy->getStateName() == "AssignedState");
    CHECK(dec->getStateName() == "AvailableState"); // Original unaffected.

    CHECK_THROWS_AS(TagDecorator(0), std::invalid_argument);
}

TEST_CASE("DepthFirstIterator visits parents before children, in insertion order") {
    SequentialStage root("root", "root");

    ParallelStage* p1 = new ParallelStage("p1", "p1");

    p1->add(newTask("a"));
    p1->add(newTask("b"));

    root.add(p1);
    root.add(newTask("c"));

    std::unique_ptr<WorkItemIterator> it(root.createIterator());

    std::vector<std::string> expected;

    expected.push_back("root");
    expected.push_back("p1");
    expected.push_back("a");
    expected.push_back("b");
    expected.push_back("c");

    CHECK(ids(it.get()) == expected);

    it->first(); // Restartable.

    CHECK(it->current()->getItemID() == "root");
}

TEST_CASE("Iterator on a leaf yields just that leaf") {
    std::unique_ptr<Task> t(newTask("solo"));

    std::unique_ptr<WorkItemIterator> it(t->createIterator());

    CHECK(ids(it.get()) == std::vector<std::string>(1, "solo"));
}

TEST_CASE("AvailableItemsIterator yields only available leaves") {
    SequentialStage root("root", "root");

    ParallelStage* par = new ParallelStage("par", "par");

    par->add(newTask("a"));
    par->add(newTask("b"));

    root.add(par);
    root.add(newTask("c"));

    {
        // Nothing available before the workflow starts.
        std::unique_ptr<WorkItemIterator> it(root.createAvailableIterator());

        CHECK(it->isDone());
    }

    root.makeAvailable();

    {
        // a and b are available; the stages and c are not reported.
        std::unique_ptr<WorkItemIterator> it(root.createAvailableIterator());

        std::vector<std::string> expected;

        expected.push_back("a");
        expected.push_back("b");

        CHECK(ids(it.get()) == expected);
    }

    Role role("General", 1);
    Participant p("p", "p", &role);

    std::vector<WorkItem*> leaves = par->getChildren();

    finish(leaves[0], &p);

    {
        // a is no longer available.
        std::unique_ptr<WorkItemIterator> it(root.createAvailableIterator());

        CHECK(ids(it.get()) == std::vector<std::string>(1, "b"));
    }
}

TEST_CASE("Iterators see through decorators without type checks") {
    SequentialStage root("root", "root");

    root.add(new TagDecorator(newTask("wrapped")));
    root.makeAvailable();

    std::unique_ptr<WorkItemIterator> it(root.createAvailableIterator());

    CHECK(ids(it.get()) == std::vector<std::string>(1, "wrapped"));
}

TEST_CASE("Memento round trip restores state, assignee and data") {
    std::unique_ptr<Task> t(newTask("t"));

    t->makeAvailable();
    t->setData("k", "before");

    std::unique_ptr<TaskMemento> m(t->createMemento());

    Role role("General", 1);
    Participant p("p", "p", &role);

    t->assign(&p);
    t->setData("k", "after");

    t->restoreMemento(m.get());

    CHECK(t->getStateName() == "AvailableState");
    CHECK(t->getAssignee() == static_cast<Participant*>(0));
    CHECK(t->getData("k") == "before");
}
