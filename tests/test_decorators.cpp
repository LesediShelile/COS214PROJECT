// Tests for the concrete Decorators: ValidatedTask, AuditedTask, PriorityTask.
#include "doctest.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AuditedTask.h"
#include "EventPublisher.h"
#include "Participant.h"
#include "PriorityTask.h"
#include "Task.h"
#include "ValidatedTask.h"
#include "WorkflowEvent.h"
#include "WorkflowListener.h"

namespace {

/** Records every event it is notified about. */
class Recorder : public WorkflowListener {
public:
    std::vector<std::string> details;
    virtual void update(WorkflowEvent* e) { details.push_back(e->getDetails()); }
};

Task* newTask() { return new Task("T1", "Triage", 0, 0); }

} // namespace

TEST_CASE("ValidatedTask blocks completion until required data is set") {
    Participant p("P1", "Dr Who");
    std::vector<std::string> req;
    req.push_back("diagnosis");
    req.push_back("disposition");
    std::unique_ptr<ValidatedTask> v(new ValidatedTask(newTask(), req));
    v->makeAvailable();
    v->assign(&p);
    v->start();

    CHECK_FALSE(v->validate());
    CHECK(v->getMissingFields().size() == 2);
    CHECK_THROWS_AS(v->complete(), std::runtime_error);
    CHECK_FALSE(v->isFinished());

    v->setData("diagnosis", "flu");
    CHECK_THROWS_AS(v->complete(), std::runtime_error);
    v->setData("disposition", "home");
    CHECK(v->validate());
    CHECK_NOTHROW(v->complete());
    CHECK(v->isFinished());
}

TEST_CASE("ValidatedTask treats empty values as missing and accepts new fields") {
    std::unique_ptr<ValidatedTask> v(new ValidatedTask(newTask(), std::vector<std::string>()));
    CHECK(v->validate());
    v->addRequiredField("note");
    v->setData("note", "");
    CHECK_FALSE(v->validate());
}

TEST_CASE("PriorityTask stores priority and mirrors it into data") {
    std::unique_ptr<PriorityTask> t(new PriorityTask(newTask(), 5));
    CHECK(t->getPriority() == 5);
    CHECK(t->getData("priority") == "5");
    t->setPriority(9);
    CHECK(t->getData("priority") == "9");
}

TEST_CASE("AuditedTask publishes one event per successful operation") {
    EventPublisher pub;
    Recorder rec;
    pub.attach(&rec);
    Participant p("P1", "Nurse");
    std::unique_ptr<AuditedTask> a(new AuditedTask(newTask(), &pub));
    a->makeAvailable();
    a->assign(&p);
    a->start();
    a->complete();
    CHECK(rec.details.size() == 4);
    CHECK(a->isFinished());
}

TEST_CASE("AuditedTask publishes nothing when the operation throws") {
    EventPublisher pub;
    Recorder rec;
    pub.attach(&rec);
    std::unique_ptr<AuditedTask> a(new AuditedTask(newTask(), &pub));
    CHECK_THROWS(a->complete()); // not started yet: illegal transition
    CHECK(rec.details.empty());
}

TEST_CASE("AuditedTask with a NULL publisher is a harmless pass-through") {
    Participant p("P1", "Nurse");
    std::unique_ptr<AuditedTask> a(new AuditedTask(newTask(), NULL));
    a->makeAvailable();
    CHECK_NOTHROW(a->assign(&p));
}

TEST_CASE("Decorators stack and clone independently") {
    EventPublisher pub;
    Recorder rec;
    pub.attach(&rec);
    Participant p("P1", "Dr");
    std::vector<std::string> req(1, "diagnosis");
    WorkItem* stacked = new AuditedTask(
        new ValidatedTask(new PriorityTask(newTask(), 3), req), &pub);
    std::unique_ptr<WorkItem> copy(stacked->clone());

    copy->makeAvailable();
    copy->assign(&p);
    copy->start();
    CHECK_THROWS(copy->complete()); // validation blocks it
    copy->setData("diagnosis", "x");
    CHECK_NOTHROW(copy->complete());
    CHECK(copy->getData("priority") == "3");

    // the original was never touched
    CHECK_FALSE(stacked->isFinished());
    CHECK_FALSE(stacked->hasData("diagnosis"));
    delete stacked;
}
