// main.cpp - the test script AND the diagram definition in one place.
// Every theatre.step(...) below becomes a node in the activity diagram. Add,
// remove or reorder steps here and the diagram changes with them; the node that
// is highlighted is the step currently running.
//
// Two scenarios run back to back in the same window:
//   Part A - Hospital admission workflow
//   Part B - Legal case workflow

#include <string>
#include <vector>
#include "include/ui/Desktop.h"
#include "include/ui/Playground.h"
#include "WorkItem.h"
#include "Task.h"
#include "SequentialStage.h"
#include "ParallelStage.h"
#include "Participant.h"
#include "Stage.h"

namespace {

// ---- helpers (shared by both scenarios) -----------------------------------
std::string state(WorkItem *w) { return w->getStateName(); }
QString qs(const std::string &s) { return QString::fromStdString(s); }

// "Released" = the parent stage has let this item start (available or beyond).
bool released(WorkItem *w) { return w->isAvailable() || state(w) != "Created"; }

// Push a task through its whole lifecycle.
void drive(WorkItem *w, Participant *who) {
    if (state(w) == "Created") w->makeAvailable();
    if (state(w) == "Available") w->assign(who);
    if (state(w) == "Assigned") w->start();
    if (state(w) == "Started") w->complete();
}

void collectLeaves(WorkItem *w, std::vector<WorkItem*> &out) {
    const std::vector<WorkItem*> kids = w->getChildren();
    if (kids.empty()) out.push_back(w);
    else for (std::size_t i = 0; i < kids.size(); ++i) collectLeaves(kids[i], out);
}

// Drives every leaf task of a tree until the root reports finished.
void runToCompletion(Desktop &theatre, Stage *root, Participant *who) {
    root->makeAvailable();
    std::vector<WorkItem*> leaves;
    collectLeaves(root, leaves);
    for (int pass = 0; pass < 50 && !root->isFinished(); ++pass) {
        root->activateChildren();
        for (std::size_t i = 0; i < leaves.size(); ++i)
            if (!leaves[i]->isFinished() && released(leaves[i])) drive(leaves[i], who);
    }
    theatre.say(QString("%1 tasks driven").arg(leaves.size()));
    Desktop::check(root->isFinished(), "root stage did not finish");
    for (std::size_t i = 0; i < leaves.size(); ++i)
        Desktop::check(state(leaves[i]) == "Completed",
                           "task not completed: " + qs(leaves[i]->getItemID()));
}

// ===========================================================================
// PART A - HOSPITAL
// ===========================================================================
// Everything the hospital steps share. Its first step resets it, so "Restart"
// in the GUI works.
struct HospitalCtx {
    Participant who;
    Stage *root = 0, *arrival = 0, *prep = 0, *admission = 0;
    Task *reg = 0, *triage = 0, *records = 0, *bay = 0, *approval = 0, *treatment = 0;
    HospitalCtx() : who("tester", "Test Runner", 0) {}
    void reset() {
        delete root;
        root = arrival = prep = admission = 0;
        reg = triage = records = bay = approval = treatment = 0;
    }
    ~HospitalCtx() { reset(); }
};

// Full hospital tree in one go, used by the end-to-end step.
void buildHospitalFull(HospitalCtx &c) {
    c.reset();
    c.root = new SequentialStage("full-root", "Hospital admission workflow");
    Stage *arrival = new SequentialStage("arrival-stage", "Arrival and triage");
    arrival->add(new Task("register", "Reception registration", 0, 0));
    arrival->add(new Task("triage", "Nurse triage and priority", 0, 0));
    Stage *prep = new ParallelStage("prep-stage", "Prepare admission in parallel");
    prep->add(new Task("records", "Retrieve patient records", 0, 0));
    prep->add(new Task("bay", "Prepare treatment bay", 0, 0));
    arrival->add(prep);
    c.root->add(arrival);
    Stage *admission = new SequentialStage("admission-stage", "Admission and treatment");
    admission->add(new Task("approval", "Request admission approval", 0, 0));
    admission->add(new Task("doctor", "Doctor accepts case", 0, 0));
    admission->add(new Task("treatment", "Assessment and treatment", 0, 0));
    admission->add(new Task("room", "Allocate room / ICU", 0, 0));
    c.root->add(admission);
    Stage *closeout = new SequentialStage("closeout-stage", "Discharge and closeout");
    closeout->add(new Task("cleaning", "Clean treatment area", 0, 0));
    closeout->add(new Task("archive", "Archive patient history", 0, 0));
    c.root->add(closeout);
}

void addHospitalScenario(Desktop &theatre, HospitalCtx &c) {
    auto &check = Desktop::check;
    auto &expectThrows = Desktop::expectThrows;

    // ------------------------------------------------------------------
    theatre.section("Hospital 1. Build the workflow (Composite)");

    theatre.step("Create the root stage", "Empty SequentialStage, no children yet", [&] {
        c.reset();
        c.root = new SequentialStage("root", "Admission workflow");
        check(c.root->getChildren().empty(), "A new stage should have no children");
        theatre.say("hospital root created");
    });

    theatre.step("Add tasks to a sequential stage", "register + triage inside 'arrival'", [&] {
        c.arrival = new SequentialStage("arrival", "Arrival and triage");
        c.reg = new Task("register", "Reception registration", 0, 0);
        c.triage = new Task("triage", "Nurse triage and priority", 0, 0);
        c.arrival->add(c.reg);
        c.arrival->add(c.triage);
        check(c.arrival->getChildren().size() == 2, "arrival should have 2 children");
    });

    theatre.step("Nest a parallel stage", "records + bay run side by side inside 'arrival'", [&] {
        c.prep = new ParallelStage("prep", "Prepare admission");
        c.records = new Task("records", "Retrieve patient records", 0, 0);
        c.bay = new Task("bay", "Prepare treatment bay", 0, 0);
        c.prep->add(c.records);
        c.prep->add(c.bay);
        c.arrival->add(c.prep);
        c.root->add(c.arrival);
        check(c.arrival->getChildren().size() == 3, "arrival should now have 3 children");
        check(c.root->getChildren().size() == 1, "root should have 1 child");
    });

    theatre.step("Add a second sequential stage", "approval -> treatment inside 'admission'", [&] {
        c.admission = new SequentialStage("admission", "Admission and treatment");
        c.approval = new Task("approval", "Request admission approval", 0, 0);
        c.treatment = new Task("treatment", "Assessment and treatment", 0, 0);
        c.admission->add(c.approval);
        c.admission->add(c.treatment);
        c.root->add(c.admission);
        check(c.root->getChildren().size() == 2, "root should have 2 stages");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 2. Initial state and sequential gating");

    theatre.step("Tasks start in Created", "Nothing is released before the workflow starts", [&] {
        check(state(c.reg) == "Created", "register should be Created, was " + qs(state(c.reg)));
        check(state(c.triage) == "Created", "triage should be Created");
    });

    theatre.step("Start the workflow", "makeAvailable() + activateChildren() releases only the first task", [&] {
        c.root->makeAvailable();
        c.root->activateChildren();
        check(released(c.reg), "register should be released first");
        check(!released(c.triage), "triage must wait for register (sequential)");
        check(!released(c.approval), "approval must wait for the arrival stage");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 3. Task lifecycle");

    theatre.step("Assign", "Available -> Assigned", [&] {
        if (state(c.reg) == "Created") c.reg->makeAvailable();
        c.reg->assign(&c.who);
        check(state(c.reg) == "Assigned", "expected Assigned, was " + qs(state(c.reg)));
    });

    theatre.step("Start", "Assigned -> Started", [&] {
        c.reg->start();
        check(state(c.reg) == "Started", "expected Started, was " + qs(state(c.reg)));
    });

    theatre.step("Complete", "Started -> Completed", [&] {
        c.reg->complete();
        check(state(c.reg) == "Completed", "expected Completed, was " + qs(state(c.reg)));
        check(c.reg->isFinished(), "a completed task should report isFinished()");
    });

    theatre.step("Next task is released", "Finishing 'register' opens 'triage'", [&] {
        c.root->activateChildren();
        check(released(c.triage), "triage should be released after register completes");
    });

    theatre.step("Illegal transition is refused", "Starting a task that is already completed must throw", [&] {
        expectThrows([&] { c.reg->start(); }, "start() on a completed task");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 4. Parallel stage");

    theatre.step("Finish triage", "Opens the parallel prep stage", [&] {
        drive(c.triage, &c.who);
        c.root->activateChildren();
        check(c.triage->isFinished(), "triage should be finished");
    });

    theatre.step("Both parallel tasks open together", "records and bay are released at the same time", [&] {
        check(released(c.records), "records should be released");
        check(released(c.bay), "bay should be released");
    });

    theatre.step("One parallel task done, stage not done", "Stage waits for every child", [&] {
        drive(c.records, &c.who);
        c.root->activateChildren();
        check(c.records->isFinished(), "records should be finished");
        check(!c.prep->isFinished(), "prep must not finish while bay is open");
    });

    theatre.step("Last parallel task finishes the stage", "prep and then arrival complete", [&] {
        drive(c.bay, &c.who);
        c.root->activateChildren();
        check(c.prep->isFinished(), "prep should be finished");
        check(c.arrival->isFinished(), "arrival should be finished");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 5. Reject and cancel");

    theatre.step("Next stage is released", "Admission opens after arrival", [&] {
        check(released(c.approval), "approval should be released once arrival is done");
    });

    theatre.step("Reject an assigned task", "reject(reason) on 'approval'", [&] {
        if (state(c.approval) == "Created") c.approval->makeAvailable();
        if (state(c.approval) == "Available") c.approval->assign(&c.who);
        c.approval->reject("Rejected by test");
        const std::string s = state(c.approval);
        theatre.say("approval is now " + qs(s));
        check(s == "Rejected" || s == "Available",
              "expected Rejected (or back to Available), was " + qs(s));
    });

    theatre.step("Cancel a task", "cancel() on 'treatment'", [&] {
        c.treatment->cancel();
        check(state(c.treatment) == "Cancelled", "expected Cancelled, was " + qs(state(c.treatment)));
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 6. End to end");

    theatre.step("Run a fresh hospital workflow to completion",
                 "Drives every task in order and checks the root finishes", [&] {
        HospitalCtx full;
        buildHospitalFull(full);
        runToCompletion(theatre, full.root, &full.who);
    });
}

// ===========================================================================
// PART B - LEGAL
// ===========================================================================
struct LegalCtx {
    Participant who;
    Stage *root = 0, *intake = 0, *discovery = 0, *litigation = 0;
    Task *client = 0, *conflict = 0, *evidence = 0, *research = 0, *draft = 0, *review = 0, *filing = 0;
    LegalCtx() : who("lawyer", "Test Attorney", 0) {}
    void reset() {
        delete root;
        root = intake = discovery = litigation = 0;
        client = conflict = evidence = research = draft = review = filing = 0;
    }
    ~LegalCtx() { reset(); }
};

// Full legal case tree in one go, used by the end-to-end step.
void buildLegalFull(LegalCtx &c) {
    c.reset();
    c.root = new SequentialStage("legal-full-root", "Legal case workflow");
    Stage *intake = new SequentialStage("intake-stage", "Client intake and conflict check");
    intake->add(new Task("client", "Client intake interview", 0, 0));
    intake->add(new Task("conflict", "Conflict of interest check", 0, 0));
    Stage *discovery = new ParallelStage("discovery-stage", "Investigate the case in parallel");
    discovery->add(new Task("evidence", "Gather evidence", 0, 0));
    discovery->add(new Task("research", "Legal research and precedents", 0, 0));
    discovery->add(new Task("witness", "Interview witnesses", 0, 0));
    intake->add(discovery);
    c.root->add(intake);
    Stage *litigation = new SequentialStage("litigation-stage", "Pleadings and court");
    litigation->add(new Task("draft", "Draft pleadings", 0, 0));
    litigation->add(new Task("review", "Partner review and sign-off", 0, 0));
    litigation->add(new Task("filing", "File with the court", 0, 0));
    litigation->add(new Task("hearing", "Attend hearing", 0, 0));
    c.root->add(litigation);
    Stage *closure = new SequentialStage("closure-stage", "Judgment and closure");
    closure->add(new Task("judgment", "Record judgment or settlement", 0, 0));
    closure->add(new Task("invoice", "Invoice the client", 0, 0));
    closure->add(new Task("archive-case", "Archive the case file", 0, 0));
    c.root->add(closure);
}

void addLegalScenario(Desktop &theatre, LegalCtx &c) {
    auto &check = Desktop::check;
    auto &expectThrows = Desktop::expectThrows;

    // ------------------------------------------------------------------
    theatre.section("Legal 1. Build the workflow (Composite)");

    theatre.step("Create the root stage", "Empty SequentialStage for a new legal case", [&] {
        c.reset();
        c.root = new SequentialStage("legal-root", "Legal case workflow");
        check(c.root->getChildren().empty(), "A new stage should have no children");
        theatre.say("legal root created");
    });

    theatre.step("Add tasks to a sequential stage", "client intake + conflict check inside 'intake'", [&] {
        c.intake = new SequentialStage("intake", "Client intake and conflict check");
        c.client = new Task("client", "Client intake interview", 0, 0);
        c.conflict = new Task("conflict", "Conflict of interest check", 0, 0);
        c.intake->add(c.client);
        c.intake->add(c.conflict);
        check(c.intake->getChildren().size() == 2, "intake should have 2 children");
    });

    theatre.step("Nest a parallel stage", "evidence + research run side by side inside 'intake'", [&] {
        c.discovery = new ParallelStage("discovery", "Investigate the case");
        c.evidence = new Task("evidence", "Gather evidence", 0, 0);
        c.research = new Task("research", "Legal research and precedents", 0, 0);
        c.discovery->add(c.evidence);
        c.discovery->add(c.research);
        c.intake->add(c.discovery);
        c.root->add(c.intake);
        check(c.intake->getChildren().size() == 3, "intake should now have 3 children");
        check(c.root->getChildren().size() == 1, "root should have 1 child");
    });

    theatre.step("Add a second sequential stage", "draft -> review -> filing inside 'litigation'", [&] {
        c.litigation = new SequentialStage("litigation", "Pleadings and court");
        c.draft = new Task("draft", "Draft pleadings", 0, 0);
        c.review = new Task("review", "Partner review and sign-off", 0, 0);
        c.filing = new Task("filing", "File with the court", 0, 0);
        c.litigation->add(c.draft);
        c.litigation->add(c.review);
        c.litigation->add(c.filing);
        c.root->add(c.litigation);
        check(c.litigation->getChildren().size() == 3, "litigation should have 3 children");
        check(c.root->getChildren().size() == 2, "root should have 2 stages");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 2. Initial state and sequential gating");

    theatre.step("Tasks start in Created", "Nothing is released before the case is opened", [&] {
        check(state(c.client) == "Created", "client intake should be Created, was " + qs(state(c.client)));
        check(state(c.conflict) == "Created", "conflict check should be Created");
    });

    theatre.step("Open the case", "makeAvailable() + activateChildren() releases only client intake", [&] {
        c.root->makeAvailable();
        c.root->activateChildren();
        check(released(c.client), "client intake should be released first");
        check(!released(c.conflict), "conflict check must wait for client intake (sequential)");
        check(!released(c.draft), "drafting must wait for the intake stage");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 3. Task lifecycle");

    theatre.step("Assign", "Available -> Assigned", [&] {
        if (state(c.client) == "Created") c.client->makeAvailable();
        c.client->assign(&c.who);
        check(state(c.client) == "Assigned", "expected Assigned, was " + qs(state(c.client)));
    });

    theatre.step("Start", "Assigned -> Started", [&] {
        c.client->start();
        check(state(c.client) == "Started", "expected Started, was " + qs(state(c.client)));
    });

    theatre.step("Complete", "Started -> Completed", [&] {
        c.client->complete();
        check(state(c.client) == "Completed", "expected Completed, was " + qs(state(c.client)));
        check(c.client->isFinished(), "a completed task should report isFinished()");
    });

    theatre.step("Next task is released", "Finishing client intake opens the conflict check", [&] {
        c.root->activateChildren();
        check(released(c.conflict), "conflict check should be released after intake completes");
    });

    theatre.step("Illegal transition is refused", "Starting a task that is already completed must throw", [&] {
        expectThrows([&] { c.client->start(); }, "start() on a completed task");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 4. Parallel discovery");

    theatre.step("Clear the conflict check", "Opens the parallel discovery stage", [&] {
        drive(c.conflict, &c.who);
        c.root->activateChildren();
        check(c.conflict->isFinished(), "conflict check should be finished");
    });

    theatre.step("Both discovery tasks open together", "evidence and research are released at the same time", [&] {
        check(released(c.evidence), "evidence gathering should be released");
        check(released(c.research), "legal research should be released");
    });

    theatre.step("One discovery task done, stage not done", "Stage waits for every child", [&] {
        drive(c.evidence, &c.who);
        c.root->activateChildren();
        check(c.evidence->isFinished(), "evidence gathering should be finished");
        check(!c.discovery->isFinished(), "discovery must not finish while research is open");
    });

    theatre.step("Last discovery task finishes the stage", "discovery and then intake complete", [&] {
        drive(c.research, &c.who);
        c.root->activateChildren();
        check(c.discovery->isFinished(), "discovery should be finished");
        check(c.intake->isFinished(), "intake should be finished");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 5. Reject and cancel");

    theatre.step("Litigation stage is released", "Drafting opens after intake", [&] {
        check(released(c.draft), "drafting should be released once intake is done");
    });

    theatre.step("Reject at partner review", "Finish the draft, then reject(reason) on 'review'", [&] {
        drive(c.draft, &c.who);
        c.root->activateChildren();
        check(c.draft->isFinished(), "draft should be finished before review");
        if (state(c.review) == "Created") c.review->makeAvailable();
        if (state(c.review) == "Available") c.review->assign(&c.who);
        c.review->reject("Pleadings need revision");
        const std::string s = state(c.review);
        theatre.say("review is now " + qs(s));
        check(s == "Rejected" || s == "Available",
              "expected Rejected (or back to Available), was " + qs(s));
    });

    theatre.step("Cancel a task", "cancel() on 'filing' - the client withdrew", [&] {
        c.filing->cancel();
        check(state(c.filing) == "Cancelled", "expected Cancelled, was " + qs(state(c.filing)));
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 6. End to end");

    theatre.step("Run a fresh legal case to completion",
                 "Drives every task in order and checks the root finishes", [&] {
        LegalCtx full;
        buildLegalFull(full);
        runToCompletion(theatre, full.root, &full.who);
    });
}

} // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Desktop theatre("TIVIDY | Workflow tests");
    HospitalCtx hospital;
    LegalCtx legal;

    theatre.tab("Hospital admission");        // Part A
    addHospitalScenario(theatre, hospital);

    theatre.tab("Legal case");                // Part B
    addLegalScenario(theatre, legal);

    Playground playground;                    // must be declared after theatre
    theatre.addTab("Playground", &playground);

    theatre.run();
    return app.exec();
}