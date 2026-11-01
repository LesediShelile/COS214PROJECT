// main.cpp - the test script AND the diagram definition in one place.
// Every theatre.step(...) below becomes a node in the activity diagram. Add,
// remove or reorder steps here and the diagram changes with them; the node that
// is highlighted is the step currently running.
//
// Two scenarios run back to back in the same window:
//   Part A - Hospital emergency patient admission (paramedic pickup to archive)
//   Part B - Legal case workflow

#include <algorithm>
#include <cstdio>
#include <ctime>
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
#include "Role.h"
#include "RoleBasedAssignment.h"
#include "WorkloadAssignment.h"
#include "RuleBasedAssignment.h"
#include "ValidatedTask.h"
#include "AuditedTask.h"
#include "PriorityTask.h"
#include "ExternalTask.h"
#include "AdapterOne.h"
#include "AdapterTwo.h"
#include "APIOne.h"
#include "APITwo.h"
#include "ApprovalLevel.h"
#include "ApprovalRequest.h"
#include "ApprovalStatus.h"
#include "EventPublisher.h"
#include "EventType.h"
#include "WorkflowEvent.h"
#include "AuditLogger.h"
#include "Notifier.h"
#include "MetricsCollector.h"
#include "CommandInvoker.h"
#include "CommandHistory.h"
#include "AssignCommand.h"
#include "StartCommand.h"
#include "CompleteCommand.h"
#include "WorkItemIterator.h"
#include "AvailableItemsIterator.h"
#include "DepthFirstIterator.h"

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

// ---- Iterator helpers (shared by both scenarios) ---------------------------
// Every task that is Available right now, found with AvailableItemsIterator.
// Call it again after any step: tasks drop out as they are assigned and
// completed, and new ones appear as the next stage opens.
std::vector<WorkItem*> availableNow(WorkItem *root) {
    std::vector<WorkItem*> out;
    WorkItemIterator *it = root->createAvailableIterator();
    for (it->first(); !it->isDone(); it->next()) out.push_back(it->current());
    delete it;
    return out;
}

std::string idList(const std::vector<WorkItem*> &v) {
    if (v.empty()) return "(none)";
    std::string s;
    for (std::size_t i = 0; i < v.size(); ++i) s += (i ? ", " : "") + v[i]->getItemID();
    return s;
}

// Walks the whole tree with DepthFirstIterator and counts every node.
int countNodes(WorkItem *root) {
    int n = 0;
    WorkItemIterator *it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) ++n;
    delete it;
    return n;
}

// Walks the tree with DepthFirstIterator and tallies the state of every task,
// e.g. "Created 9 | Available 2 | Completed 1".
std::string stateTally(WorkItem *root) {
    static const char *names[] = {"Created", "Available", "Assigned", "Started",
                                  "Completed", "Rejected", "Cancelled"};
    int counts[7] = {0, 0, 0, 0, 0, 0, 0};
    WorkItemIterator *it = root->createIterator();
    for (it->first(); !it->isDone(); it->next()) {
        WorkItem *w = it->current();
        if (!w->getChildren().empty()) continue;          // stages are not tasks
        const std::string s = w->getStateName();
        for (int i = 0; i < 7; ++i) if (s == names[i]) ++counts[i];
    }
    delete it;
    std::string out;
    for (int i = 0; i < 7; ++i) {
        if (!counts[i]) continue;
        if (!out.empty()) out += " | ";
        out += std::string(names[i]) + " " + std::to_string(counts[i]);
    }
    return out.empty() ? "(no tasks)" : out;
}

void sayAvailable(Desktop &theatre, WorkItem *root) {
    const std::vector<WorkItem*> a = availableNow(root);
    theatre.say(QString("iterator: %1 available -> %2").arg(a.size()).arg(qs(idList(a))));
}

// ---- shared by both scenarios: timestamps, log search, tree search ----------
std::string stamp() {
    char buf[32];
    std::time_t t = std::time(0);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

bool anyContains(const std::vector<std::string> &lines, const std::string &needle) {
    for (std::size_t i = 0; i < lines.size(); ++i)
        if (lines[i].find(needle) != std::string::npos) return true;
    return false;
}

WorkItem *findItem(WorkItem *w, const std::string &id) {
    if (!w) return 0;
    if (w->getItemID() == id) return w;
    const std::vector<WorkItem*> kids = w->getChildren();
    for (std::size_t i = 0; i < kids.size(); ++i)
        if (WorkItem *hit = findItem(kids[i], id)) return hit;
    return 0;
}

// ===========================================================================
// PART A - HOSPITAL: EMERGENCY PATIENT ADMISSION
//
// One ambulance patient, from paramedic pickup to cleaning and archiving:
//
//   Transport and bay preparation (parallel)
//       Ambulance run (sequential): paramedic pickup (dispatch is told) -> transport
//       Nurse prepares the treatment bay while the ambulance is on its way
//   Arrival and intake (sequential)
//       Secretary registers the patient -> patient history is retrieved ->
//       paramedic hands over -> nurse intake (triage priority, audited)
//   Assessment and treatment (sequential)
//       Doctor accepts the case (deadline + reroute) -> assessment -> room allocation
//       (ICU needs the Department Head, surgery needs the Hospital Administrator)
//   Closeout (sequential)
//       Cleaning (only after the patient has left) -> archive the history
//
// Failure paths: delayed ambulance, doctor who does not respond, patient who
// deteriorates during intake, patient who does not survive.
// External systems: ambulance dispatch (APITwo via AdapterTwo) and hospital
// patient records (APIOne via AdapterOne).
// ===========================================================================

// Everything the hospital needs that is NOT the workflow tree itself: the
// staff, their roles, the assignment strategies, the approval chain, the
// event bus with its listeners, and the two external systems with adapters.
struct HospitalWorld {
    // roles (the number is the authority level used by the approval chain)
    Role paramedicRole, secretaryRole, nurseRole, doctorRole,
         deptHeadRole, adminRole, cleanerRole;

    // staff
    Participant paramedic, secretary, nurseDlamini, nurseKhoza,
                drKhumalo, drNaidoo, cleaner;

    // Strategy: who gets the next task
    RoleBasedAssignment byRole;
    WorkloadAssignment byWorkload;
    RuleBasedAssignment lightLoadDoctor;   // a doctor with fewer than 2 open cases

    // Observer: one bus, three independent listeners
    EventPublisher bus;
    AuditLogger audit;
    Notifier notifier;
    MetricsCollector metrics;

    // Adapter: the two external systems the hospital talks to
    APIOne patientRecords;
    APITwo ambulanceDispatch;
    AdapterOne recordsAdapter;
    AdapterTwo dispatchAdapter;

    // Chain of Responsibility: Doctor -> Department Head -> Hospital Administrator.
    // The head of the chain owns the rest, so only 'chain' is ever deleted.
    ApprovalLevel *doctorLevel, *headLevel, *adminLevel, *chain;

    HospitalWorld()
        : paramedicRole("Paramedic", 1), secretaryRole("Secretary", 1),
          nurseRole("Nurse", 2), doctorRole("Doctor", 3),
          deptHeadRole("Department Head", 4), adminRole("Hospital Administrator", 5),
          cleanerRole("Cleaner", 1),
          paramedic("PM1", "Paramedic Thabo", &paramedicRole),
          secretary("SEC1", "Secretary Lerato", &secretaryRole),
          nurseDlamini("N1", "Nurse Dlamini", &nurseRole),
          nurseKhoza("N2", "Nurse Khoza", &nurseRole),
          drKhumalo("DR1", "Dr Khumalo", &doctorRole),
          drNaidoo("DR2", "Dr Naidoo", &doctorRole),
          cleaner("CL1", "Cleaner Pieter", &cleanerRole),
          lightLoadDoctor([this](Task*, Participant *p) {
              return p->hasRole(&doctorRole) && p->getWorkload() < 2;
          }),
          recordsAdapter(&patientRecords),
          dispatchAdapter(&ambulanceDispatch) {
        doctorLevel = new ApprovalLevel(&doctorRole, 1800);
        headLevel   = new ApprovalLevel(&deptHeadRole, 1800);
        adminLevel  = new ApprovalLevel(&adminRole, 3600);
        doctorLevel->setNext(headLevel);
        headLevel->setNext(adminLevel);
        chain = doctorLevel;

        bus.attach(&audit);
        bus.attach(&notifier);
        bus.attach(&metrics);
    }
    ~HospitalWorld() { delete chain; }

    std::vector<Participant*> staff() {
        std::vector<Participant*> v;
        v.push_back(&paramedic);    v.push_back(&secretary);
        v.push_back(&nurseDlamini); v.push_back(&nurseKhoza);
        v.push_back(&drKhumalo);    v.push_back(&drNaidoo);
        v.push_back(&cleaner);
        return v;
    }
};

// Raw task pointers, so a step can reach a task without searching the tree.
// 'xxxW' is the item as it sits in the tree (possibly wrapped in decorators).
struct Pieces {
    Stage *transport, *ambulance, *arrival, *treatment, *closeout;
    ExternalTask *pickup;
    Task *transportT, *bay, *regT, *records, *handover, *intakeT,
         *accept, *assessT, *room, *cleaning, *archive;
    WorkItem *regW, *intakeW, *assessW;
    Pieces() : transport(0), ambulance(0), arrival(0), treatment(0), closeout(0),
               pickup(0), transportT(0), bay(0), regT(0), records(0), handover(0),
               intakeT(0), accept(0), assessT(0), room(0), cleaning(0), archive(0),
               regW(0), intakeW(0), assessW(0) {}
};

// ---- the four stages of the emergency admission -----------------------------

// Transport and bay preparation run side by side.
Stage *makeTransportStage(HospitalWorld &w, Pieces &p) {
    p.transport = new ParallelStage("transport-stage", "Transport and bay preparation");
    p.ambulance = new SequentialStage("ambulance", "Ambulance run");
    p.pickup = new ExternalTask("pickup", "Paramedic picks up patient (dispatch notified)",
                                &w.paramedicRole, &w.byRole, &w.dispatchAdapter, "dispatch");
    p.transportT = new Task("transport", "Transport patient to hospital",
                            &w.paramedicRole, &w.byRole);
    p.ambulance->add(p.pickup);
    p.ambulance->add(p.transportT);
    p.bay = new Task("bay-prep", "Nurse prepares treatment bay",
                     &w.nurseRole, &w.byWorkload);
    p.transport->add(p.ambulance);
    p.transport->add(p.bay);
    return p.transport;
}

// Registration, patient history, handover, nurse intake: strictly in order.
Stage *makeArrivalStage(HospitalWorld &w, Pieces &p) {
    p.arrival = new SequentialStage("arrival-stage", "Arrival and intake");

    p.regT = new Task("register", "Secretary registers patient",
                      &w.secretaryRole, &w.byRole);
    std::vector<std::string> required;
    required.push_back("patientName");
    required.push_back("complaint");
    p.regW = new ValidatedTask(p.regT, required);            // Decorator: needs patient data

    p.records = new Task("records", "Retrieve patient history from hospital records",
                         &w.secretaryRole, &w.byRole);
    p.handover = new Task("handover", "Paramedic hands patient over to nurse",
                          &w.paramedicRole, &w.byRole);

    p.intakeT = new Task("intake", "Nurse intake and preparation",
                         &w.nurseRole, &w.byWorkload);
    WorkItem *audited = new AuditedTask(p.intakeT, &w.bus);  // Decorator: audited ...
    p.intakeW = new PriorityTask(audited, 1);                // ... and stacked with priority 1 = critical

    p.arrival->add(p.regW);
    p.arrival->add(p.records);
    p.arrival->add(p.handover);
    p.arrival->add(p.intakeW);
    return p.arrival;
}

// Doctor accepts (with a deadline), assesses, then allocates a room.
Stage *makeTreatmentStage(HospitalWorld &w, Pieces &p) {
    p.treatment = new SequentialStage("treatment-stage", "Assessment and treatment");
    p.accept = new Task("doctor-accept", "Doctor accepts the case",
                        &w.doctorRole, &w.byWorkload);
    p.assessT = new Task("assessment", "Doctor assesses and treats patient",
                         &w.doctorRole, &w.lightLoadDoctor);
    p.assessW = new AuditedTask(p.assessT, &w.bus);
    p.room = new Task("room-allocation", "Doctor allocates room (ward / ICU / surgery)",
                      &w.doctorRole, &w.byRole);
    p.treatment->add(p.accept);
    p.treatment->add(p.assessW);
    p.treatment->add(p.room);
    return p.treatment;
}

// Cleaning only starts once the patient has left the area.
Stage *makeCloseoutStage(HospitalWorld &w, Pieces &p) {
    p.closeout = new SequentialStage("closeout-stage", "Cleaning and closeout");
    p.cleaning = new Task("cleaning", "Cleaning staff clean the bay",
                          &w.cleanerRole, &w.byRole);
    p.archive = new Task("archive", "Secretary archives patient history",
                         &w.secretaryRole, &w.byRole);
    p.closeout->add(p.cleaning);
    p.closeout->add(p.archive);
    return p.closeout;
}

Stage *buildEmergencyAdmission(HospitalWorld &w, Pieces &p) {
    Stage *root = new SequentialStage("emergency-admission", "Emergency patient admission");
    root->add(makeTransportStage(w, p));
    root->add(makeArrivalStage(w, p));
    root->add(makeTreatmentStage(w, p));
    root->add(makeCloseoutStage(w, p));
    return root;
}

// Everything the hospital steps share. Its first step resets it, so "Restart"
// in the GUI works. The tree is always deleted before the world it points into.
struct HospitalCtx {
    HospitalWorld *w;
    Stage *root;          // patient A, the case we walk through step by step
    WorkItem *patientB;   // a second patient cloned from the same definition
    Pieces p;
    HospitalCtx() : w(0), root(0), patientB(0) {}
    void reset() {
        delete patientB;
        delete root;
        delete w;
        patientB = 0;
        root = 0;
        w = 0;
        p = Pieces();
    }
    ~HospitalCtx() { reset(); }
};

void addHospitalScenario(Desktop &theatre, HospitalCtx &c) {
    auto &check = Desktop::check;
    auto &expectThrows = Desktop::expectThrows;

    // ------------------------------------------------------------------
    theatre.section("Hospital 1. Staff, roles and assignment (Strategy)");

    theatre.step("Create the hospital", "Paramedic, secretary, 2 nurses, 2 doctors, cleaner, plus Department Head and Administrator roles", [&] {
        c.reset();
        c.w = new HospitalWorld();
        check(c.w->paramedic.hasRole(&c.w->paramedicRole), "paramedic should hold the Paramedic role");
        check(c.w->nurseRole.getAuthorityLevel() < c.w->doctorRole.getAuthorityLevel(),
              "a doctor outranks a nurse");
        check(c.w->doctorRole.getAuthorityLevel() < c.w->deptHeadRole.getAuthorityLevel(),
              "the Department Head outranks a doctor");
        check(c.w->deptHeadRole.getAuthorityLevel() < c.w->adminRole.getAuthorityLevel(),
              "the Administrator outranks the Department Head");
        theatre.say("7 staff, 7 roles, approval authority 1 to 5");
    });

    theatre.step("Role-based: a nursing task goes to a nurse", "RoleBasedAssignment skips the paramedic and secretary", [&] {
        Task probe("probe", "Probe nursing task", &c.w->nurseRole, &c.w->byRole);
        Participant *who = c.w->byRole.selectParticipant(&probe, c.w->staff());
        check(who != 0, "someone should be selected");
        check(who->hasRole(&c.w->nurseRole), "selected " + qs(who->getName()) + " is not a nurse");
    });

    theatre.step("Workload-based: the least busy nurse is chosen", "Nurse Dlamini has 3 patients, Nurse Khoza has none", [&] {
        c.w->nurseDlamini.incrementWorkload();
        c.w->nurseDlamini.incrementWorkload();
        c.w->nurseDlamini.incrementWorkload();
        Task probe("probe", "Probe nursing task", &c.w->nurseRole, &c.w->byWorkload);
        Participant *who = c.w->byWorkload.selectParticipant(&probe, c.w->staff());
        check(who == &c.w->nurseKhoza, "expected Nurse Khoza, got " + qs(who ? who->getName() : "nobody"));
    });

    theatre.step("Rule-based: skip a doctor with 2 open cases", "Rule: a doctor with fewer than 2 open cases", [&] {
        c.w->drKhumalo.incrementWorkload();
        c.w->drKhumalo.incrementWorkload();
        Task probe("probe", "Probe doctor task", &c.w->doctorRole, &c.w->lightLoadDoctor);
        Participant *who = c.w->lightLoadDoctor.selectParticipant(&probe, c.w->staff());
        c.w->drKhumalo.decrementWorkload();
        c.w->drKhumalo.decrementWorkload();
        check(who == &c.w->drNaidoo, "expected Dr Naidoo, got " + qs(who ? who->getName() : "nobody"));
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 2. Build the emergency admission (Composite + Decorator)");

    theatre.step("Create the root stage", "Empty SequentialStage 'Emergency patient admission'", [&] {
        c.root = new SequentialStage("emergency-admission", "Emergency patient admission");
        check(c.root->getChildren().empty(), "A new stage should have no children");
    });

    theatre.step("Add transport and bay preparation", "ParallelStage: [ambulance run (pickup -> transport)] || [bay prep]", [&] {
        c.root->add(makeTransportStage(*c.w, c.p));
        check(c.p.transport->getChildren().size() == 2, "transport stage: ambulance run + bay prep");
        check(c.p.ambulance->getChildren().size() == 2, "ambulance run: pickup + transport");
    });

    theatre.step("Add arrival and intake", "register -> patient history -> handover -> nurse intake", [&] {
        c.root->add(makeArrivalStage(*c.w, c.p));
        check(c.p.arrival->getChildren().size() == 4, "arrival stage should hold 4 tasks");
    });

    theatre.step("Add assessment and treatment", "doctor accepts -> assessment -> room allocation", [&] {
        c.root->add(makeTreatmentStage(*c.w, c.p));
        check(c.p.treatment->getChildren().size() == 3, "treatment stage should hold 3 tasks");
    });

    theatre.step("Add cleaning and closeout", "cleaning -> archive history", [&] {
        c.root->add(makeCloseoutStage(*c.w, c.p));
        check(c.p.closeout->getChildren().size() == 2, "closeout stage should hold 2 tasks");
        check(c.root->getChildren().size() == 4, "root should hold 4 stages");
    });

    theatre.step("Decorators wrap tasks without new task types", "Registration is validated, intake is audited + priority 1, assessment is audited", [&] {
        check(dynamic_cast<ValidatedTask*>(c.p.regW) != 0, "registration should be a ValidatedTask");
        PriorityTask *pr = dynamic_cast<PriorityTask*>(c.p.intakeW);
        check(pr != 0, "intake should be a PriorityTask");
        check(pr->getPriority() == 1, "intake priority should be 1 (critical)");
        check(dynamic_cast<AuditedTask*>(pr->getWrapped()) != 0, "intake should also be audited (stacked)");
        check(dynamic_cast<AuditedTask*>(c.p.assessW) != 0, "assessment should be audited");
        check(c.p.regW->getItemID() == "register", "a decorator keeps the task's id");
    });

    theatre.step("Count the work", "12 tasks across 4 stages", [&] {
        std::vector<WorkItem*> leaves;
        collectLeaves(c.root, leaves);
        check(leaves.size() == 12, QString("expected 12 tasks, found %1").arg(leaves.size()));
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 3. Two patients, one definition (Prototype)");

    theatre.step("Clone the workflow for a second patient", "root->clone() copies the whole tree: stages, tasks and decorators", [&] {
        c.patientB = c.root->clone();
        check(c.patientB != c.root, "the clone must be a different object");
        check(c.patientB->getChildren().size() == c.root->getChildren().size(), "same shape as the original");
        check(findItem(c.patientB, "pickup") != findItem(c.root, "pickup"), "every task must be its own copy");
    });

    theatre.step("Patient B moves, patient A does not", "Driving B's pickup leaves A's pickup in Created", [&] {
        c.patientB->makeAvailable();
        c.patientB->activateChildren();
        WorkItem *bPickup = findItem(c.patientB, "pickup");
        drive(bPickup, &c.w->paramedic);
        check(bPickup->isFinished(), "B's pickup should be finished");
        check(state(c.p.pickup) == "Created", "A's pickup must still be Created, was " + qs(state(c.p.pickup)));
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 4. Pickup and transport (Parallel stage + Adapter)");

    theatre.step("Ambulance dispatched: start patient A", "Pickup and bay preparation open together; transport waits for pickup", [&] {
        c.root->makeAvailable();
        c.root->activateChildren();
        check(released(c.p.pickup), "pickup should be released");
        check(released(c.p.bay), "bay preparation runs in parallel, so it is released too");
        check(!released(c.p.transportT), "transport must wait for the pickup (sequential)");
        check(!released(c.p.regT), "registration must wait for the transport stage");
        sayAvailable(theatre, c.root);
        check(availableNow(c.root).size() == 2, "the iterator should find exactly pickup and bay-prep");
    });

    theatre.step("Paramedic picks the patient up", "Assigned -> Started", [&] {
        c.p.pickup->assign(&c.w->paramedic);
        c.p.pickup->start();
        check(state(c.p.pickup) == "Started", "expected Started, was " + qs(state(c.p.pickup)));
    });

    theatre.step("Ambulance dispatch is notified (Adapter)", "ExternalTask -> AdapterTwo -> APITwo dispatch", [&] {
        check(c.p.pickup->execute(), "the dispatch request should be accepted");
        check(c.p.pickup->getResponse() == "Request submitted to API Two",
              "unexpected dispatch response: " + qs(c.p.pickup->getResponse()));
        check(c.w->dispatchAdapter.getName() == "API Two", "dispatch adapter should front APITwo");
        c.p.pickup->complete();
        check(c.p.pickup->isFinished(), "pickup should be complete");
    });

    theatre.step("Transport opens after pickup", "Finishing 'pickup' releases 'transport'", [&] {
        c.root->activateChildren();
        check(released(c.p.transportT), "transport should be released after pickup");
        sayAvailable(theatre, c.root);
        check(availableNow(c.root).size() == 2, "the iterator should now find transport and bay-prep");
    });

    theatre.step("Nurse prepares the bay while the ambulance drives", "Workload strategy picks Nurse Khoza; the iterator count drops from 2 to 1", [&] {
        c.p.bay->autoAssign(c.w->staff());
        check(c.p.bay->getAssignee() == &c.w->nurseKhoza,
              "expected Nurse Khoza, got " + qs(c.p.bay->getAssignee() ? c.p.bay->getAssignee()->getName() : "nobody"));
        c.p.bay->start();
        sayAvailable(theatre, c.root);
        check(availableNow(c.root).size() == 1, "bay-prep is being worked on, so only transport is still waiting");
        c.p.bay->complete();
        check(c.p.bay->isFinished(), "bay should be ready");
        check(!c.p.transport->isFinished(), "the stage waits for the slowest branch (the ambulance)");
    });

    theatre.step("Ambulance arrives", "Transport completes, the parallel stage finishes, arrival opens", [&] {
        drive(c.p.transportT, &c.w->paramedic);
        c.root->activateChildren();
        check(c.p.transport->isFinished(), "transport stage should be finished");
        check(released(c.p.regT), "registration should be released on arrival");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 5. Arrival and nurse intake (Decorator + Strategy + Adapter)");

    theatre.step("Registration refuses to complete without patient data", "ValidatedTask throws until patientName and complaint are set", [&] {
        c.p.regW->assign(&c.w->secretary);
        c.p.regW->start();
        expectThrows([&] { c.p.regW->complete(); }, "complete() with no patient data");
        check(state(c.p.regW) == "Started", "a refused completion must leave the task Started");
    });

    theatre.step("Secretary registers the patient", "Fill patientName and complaint, then complete", [&] {
        c.p.regW->setData("patientName", "Unknown male, approx. 50");
        c.p.regW->setData("complaint", "Chest pain");
        c.p.regW->complete();
        check(state(c.p.regW) == "Completed", "expected Completed, was " + qs(state(c.p.regW)));
        c.root->activateChildren();
        check(released(c.p.records), "patient history should be released after registration");
        sayAvailable(theatre, c.root);
        check(availableNow(c.root).size() == 1, "only the patient-history task should be available now");
    });

    theatre.step("Patient history is retrieved (Adapter)", "AdapterOne -> APIOne: add patient-1042 to hospital records", [&] {
        std::string operation = "add";
        std::string payload = "patient-1042=chest pain on arrival";
        c.w->recordsAdapter.request(&operation, &payload);   // throws if the adapter rejects it
        check(c.w->recordsAdapter.getName() == "API One", "records adapter should front APIOne");
        drive(c.p.records, &c.w->secretary);
        check(c.p.records->isFinished(), "history retrieval should be complete");
    });

    theatre.step("Handover: intake cannot begin before it", "Nurse intake stays locked until the paramedic hands over", [&] {
        c.root->activateChildren();
        check(released(c.p.handover), "handover should be released");
        check(!released(c.p.intakeT), "intake must wait for the handover");
        drive(c.p.handover, &c.w->paramedic);
        c.root->activateChildren();
        check(released(c.p.intakeT), "intake should open once the handover is complete");
    });

    theatre.step("Intake is critical priority and audited", "PriorityTask(AuditedTask(Task)): priority 1 stored on the item", [&] {
        PriorityTask *pr = dynamic_cast<PriorityTask*>(c.p.intakeW);
        check(pr != 0 && pr->getPriority() == 1, "intake should carry priority 1");
        check(c.p.intakeW->getData("priority") == "1", "priority should be stored as data");
    });

    theatre.step("Intake goes to the least busy nurse", "Strategy picks Nurse Khoza; the audited decorator logs the assignment", [&] {
        const std::size_t before = c.w->audit.getRecords().size();
        Participant *who = c.w->byWorkload.selectParticipant(c.p.intakeT, c.w->staff());
        check(who == &c.w->nurseKhoza, "expected Nurse Khoza, got " + qs(who ? who->getName() : "nobody"));
        c.p.intakeW->assign(who);
        c.p.intakeW->start();
        c.p.intakeW->complete();
        check(state(c.p.intakeW) == "Completed", "expected Completed, was " + qs(state(c.p.intakeW)));
        check(c.w->audit.getRecords().size() > before, "the audited decorator should have logged the intake");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 6. Doctor acceptance, deadline and reroute (Command + Memento + State)");

    theatre.step("Treatment opens", "The doctor's acceptance is the first task of the next stage", [&] {
        c.root->activateChildren();
        check(released(c.p.accept), "doctor acceptance should be released");
        check(!released(c.p.assessT), "assessment must wait for the doctor to accept");
    });

    theatre.step("Wrong doctor assigned, then undone", "AssignCommand snapshots the task (Memento); undo() restores it", [&] {
        CommandInvoker invoker;
        invoker.executeCommand(new AssignCommand(c.p.accept, &c.w->drNaidoo));
        check(state(c.p.accept) == "Assigned", "expected Assigned, was " + qs(state(c.p.accept)));
        check(invoker.canUndo(), "the assignment should be undoable");
        check(invoker.undoLast(), "undo should succeed");
        check(state(c.p.accept) == "Available", "undo should restore Available, was " + qs(state(c.p.accept)));
        check(c.p.accept->getAssignee() == 0, "undo should clear the assignee");
    });

    theatre.step("Dr Khumalo is assigned but does not respond", "Deadline passes, the hospital is told via a TIMEOUT event", [&] {
        c.p.accept->assign(&c.w->drKhumalo);
        c.w->drKhumalo.incrementWorkload();
        c.p.accept->setDeadline(std::time(0) - 60);
        check(c.p.accept->getDeadline() < std::time(0), "the acceptance deadline should have passed");
        WorkflowEvent timeout(EventType::TIMEOUT, "doctor-accept", stamp(),
                              "Dr Khumalo did not accept the case before the deadline");
        c.w->bus.publish(&timeout);
        check(anyContains(c.w->notifier.getNotifications(), "Approval timeout"),
              "the notifier should report the timeout");
    });

    theatre.step("Case is rerouted to the other doctor", "reject -> Rejected -> makeAvailable -> workload strategy picks Dr Naidoo", [&] {
        c.p.accept->reject("No response within the deadline");
        check(state(c.p.accept) == "Rejected", "expected Rejected, was " + qs(state(c.p.accept)));
        check(c.p.accept->getAssignee() == 0, "a rejected task should have no assignee");
        check(c.p.accept->getData("rejectionReason") == "No response within the deadline",
              "the rejection reason should be stored");
        c.p.accept->makeAvailable();
        check(state(c.p.accept) == "Available", "the case should be available again");
        c.p.accept->autoAssign(c.w->staff());
        check(c.p.accept->getAssignee() == &c.w->drNaidoo,
              "expected Dr Naidoo, got " + qs(c.p.accept->getAssignee() ? c.p.accept->getAssignee()->getName() : "nobody"));
    });

    theatre.step("Dr Naidoo accepts the case", "Started -> Completed; assessment is released", [&] {
        c.p.accept->start();
        c.p.accept->complete();
        c.root->activateChildren();
        check(c.p.accept->isFinished(), "acceptance should be complete");
        check(released(c.p.assessT), "assessment should be released after acceptance");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 7. Assessment and room approval (Chain of Responsibility)");

    theatre.step("A lightly loaded doctor assesses and treats the patient", "Rule-based assignment; assign, start and complete are all audited", [&] {
        const std::size_t before = c.w->audit.getRecords().size();
        Participant *who = c.w->lightLoadDoctor.selectParticipant(c.p.assessT, c.w->staff());
        check(who != 0 && who->hasRole(&c.w->doctorRole), "a doctor should be selected");
        c.p.assessW->assign(who);
        c.p.assessW->start();
        c.p.assessW->complete();
        check(state(c.p.assessW) == "Completed", "expected Completed, was " + qs(state(c.p.assessW)));
        check(anyContains(c.w->audit.getRecords(), "was completed"), "the audit log should show the completion");
        check(c.w->audit.getRecords().size() >= before + 3, "assign, start and complete should each be logged");
        c.root->activateChildren();
    });

    theatre.step("General room: the doctor can approve", "Required level 3, doctor authority 3", [&] {
        ApprovalRequest request(c.p.room, 3, std::time(0) + 1800);
        check(c.w->doctorLevel->canApprove(&request), "a doctor should be able to approve a general room");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the request should be Approved");
    });

    theatre.step("ICU transfer: escalates to the Department Head", "Required level 4: doctor cannot approve, Department Head can", [&] {
        ApprovalRequest request(c.p.room, 4, std::time(0) + 1800);
        check(!c.w->doctorLevel->canApprove(&request), "a doctor must not approve an ICU transfer");
        check(c.w->headLevel->canApprove(&request), "the Department Head can approve an ICU transfer");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the request should end up Approved");
    });

    theatre.step("Surgery: escalates to the Hospital Administrator", "Required level 5: only the top of the chain can approve", [&] {
        ApprovalRequest request(c.p.room, 5, std::time(0) + 3600);
        check(!c.w->doctorLevel->canApprove(&request), "a doctor must not approve surgery");
        check(!c.w->headLevel->canApprove(&request), "the Department Head must not approve surgery");
        check(c.w->adminLevel->canApprove(&request), "the Administrator can approve surgery");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the request should end up Approved");
    });

    theatre.step("A rejected request stays rejected", "Once decided, later handlers cannot override it", [&] {
        ApprovalRequest request(c.p.room, 4, std::time(0) + 1800);
        request.reject(&c.w->deptHeadRole);
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Rejected, "a rejected request must stay Rejected");
    });

    theatre.step("An unanswered approval expires and escalates", "Deadline passed -> isExpired -> the chain resets the deadline and decides", [&] {
        ApprovalRequest request(c.p.room, 4, std::time(0) - 60);
        check(request.isExpired(std::time(0)), "an undecided request past its deadline is expired");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the escalated request should be Approved");
        check(!request.isExpired(std::time(0)), "a decided request is no longer expired");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 8. Room allocation, cleaning and archive");

    theatre.step("Cleaning is locked until the patient leaves", "Cleaning waits for room allocation", [&] {
        check(!released(c.p.cleaning), "cleaning must not start while the patient is still in the area");
    });

    theatre.step("Doctor allocates the ICU (approved)", "The patient is moved; cleaning opens", [&] {
        drive(c.p.room, &c.w->drNaidoo);
        c.root->activateChildren();
        check(c.p.room->isFinished(), "room allocation should be complete");
        check(released(c.p.cleaning), "cleaning should open once the patient has left");
    });

    theatre.step("Cleaning staff clean the bay", "Created -> Completed", [&] {
        drive(c.p.cleaning, &c.w->cleaner);
        c.root->activateChildren();
        check(c.p.cleaning->isFinished(), "cleaning should be complete");
        check(released(c.p.archive), "archiving should open after cleaning");
    });

    theatre.step("Secretary archives the history", "Whole workflow finishes: transferred to ICU", [&] {
        drive(c.p.archive, &c.w->secretary);
        c.root->activateChildren();
        check(c.root->isFinished(), "the whole admission should be finished");
        c.root->setData("outcome", "Transferred to ICU");
        check(c.root->getData("outcome") == "Transferred to ICU", "the outcome should be recorded");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 9. Audit, notifications and metrics (Observer)");

    theatre.step("Audit log and notifier saw the same events", "One bus, independent listeners", [&] {
        const std::size_t audited = c.w->audit.getRecords().size();
        check(audited > 0, "the audit log should not be empty");
        check(static_cast<std::size_t>(c.w->notifier.getNotificationCount()) == audited,
              "every published event should reach both listeners");
    });

    theatre.step("Ambulance dispatch reports an external event", "EXTERNAL_EVENT reaches the audit log and the notifier", [&] {
        const int before = c.w->notifier.getNotificationCount();
        WorkflowEvent external(EventType::EXTERNAL_EVENT, "ambulance-12", stamp(),
                               "Ambulance 12 is en route with the patient");
        c.w->bus.publish(&external);
        check(c.w->notifier.getNotificationCount() == before + 1, "the notifier should receive the event");
        check(anyContains(c.w->notifier.getNotifications(), "External event received"),
              "the notification should name an external event");
        check(anyContains(c.w->audit.getRecords(), "Ambulance 12 is en route"),
              "the audit log should record the event details");
    });

    theatre.step("Metrics: how long did treatment take?", "Two state changes 12 minutes apart give an average of 720 s", [&] {
        WorkflowEvent started(EventType::ITEM_STATE_CHANGED, "assessment", "2026-10-31 10:00:00", "Started");
        WorkflowEvent completed(EventType::ITEM_STATE_CHANGED, "assessment", "2026-10-31 10:12:00", "Completed");
        c.w->bus.publish(&started);
        c.w->bus.publish(&completed);
        check(c.w->metrics.getTransitionCount("Started") == 1, "one transition into Started");
        const float avg = c.w->metrics.getAverageTimeInState("Started");
        check(avg > 719.9f && avg < 720.1f, QString("expected 720 s in Started, got %1").arg(avg));
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 10. Undo history (Command + Memento)");

    theatre.step("Patient B's bay: assign, start, complete", "Three commands, three snapshots", [&] {
        WorkItem *bBay = findItem(c.patientB, "bay-prep");
        check(released(bBay), "B's bay should be released");
        CommandInvoker invoker;
        invoker.executeCommand(new AssignCommand(bBay, &c.w->nurseKhoza));
        invoker.executeCommand(new StartCommand(bBay));
        invoker.executeCommand(new CompleteCommand(bBay));
        check(state(bBay) == "Completed", "expected Completed, was " + qs(state(bBay)));
        check(invoker.getHistory()->size() == 3, "three commands should be in the history");

        check(invoker.undoLast(), "first undo");
        check(state(bBay) == "Started", "undoing Complete should give Started, was " + qs(state(bBay)));
        check(invoker.undoLast(), "second undo");
        check(state(bBay) == "Assigned", "undoing Start should give Assigned, was " + qs(state(bBay)));
        check(invoker.undoLast(), "third undo");
        check(state(bBay) == "Available", "undoing Assign should give Available, was " + qs(state(bBay)));
        check(!invoker.canUndo(), "the history should now be empty");
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 11. Work moving from Created to Completed (Iterator)");

    theatre.step("Walk the whole tree (DepthFirstIterator)", "18 nodes: 6 stages + 12 tasks, every task still Created", [&] {
        Pieces p;
        Stage *t = buildEmergencyAdmission(*c.w, p);
        check(countNodes(t) == 18, QString("expected 18 nodes, walked %1").arg(countNodes(t)));
        theatre.say("tally: " + qs(stateTally(t)));
        check(stateTally(t) == "Created 12", "every task should start Created, was " + qs(stateTally(t)));
        delete t;
    });

    theatre.step("Drain the work with AvailableItemsIterator", "Ask the iterator what is available, complete one task, ask again, until nothing is left", [&] {
        Pieces p;
        Stage *t = buildEmergencyAdmission(*c.w, p);
        p.regW->setData("patientName", "Unknown male, approx. 40");
        p.regW->setData("complaint", "Road traffic accident");
        t->makeAvailable();

        std::vector<std::size_t> sizes;
        int passes = 0;
        for (; passes < 40; ++passes) {
            std::vector<WorkItem*> avail = availableNow(t);
            if (avail.empty()) break;
            sizes.push_back(avail.size());
            theatre.say(QString("pass %1: %2 available [%3] | %4")
                            .arg(passes + 1).arg(avail.size()).arg(qs(idList(avail))).arg(qs(stateTally(t))));
            drive(avail[0], &c.w->nurseKhoza);          // first available task goes Available -> Completed
            t->activateChildren();                       // next tasks are released
        }
        theatre.say("done: " + qs(stateTally(t)));
        check(passes == 12, QString("expected 12 passes (one per task), took %1").arg(passes));
        check(sizes.front() == 2, "the parallel stage opens with 2 available tasks");
        check(sizes.back() == 1, "the last task is the only one left");
        check(availableNow(t).empty(), "nothing should be available at the end");
        check(stateTally(t) == "Completed 12", "every task should be Completed, was " + qs(stateTally(t)));
        check(t->isFinished(), "the root should be finished");
        delete t;
    });

    theatre.step("Patient A is done, patient B still has work", "The iterator sees each clone's own state (Prototype)", [&] {
        check(availableNow(c.root).empty(), "finished patient A should have nothing available");
        check(stateTally(c.root) == "Completed 12", "patient A should be Completed 12, was " + qs(stateTally(c.root)));
        c.patientB->activateChildren();
        sayAvailable(theatre, c.patientB);
        check(availableNow(c.patientB).size() == 2, "patient B should have transport and bay-prep available");
        theatre.say("patient B: " + qs(stateTally(c.patientB)));
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 12. Failure paths");

    theatre.step("Ambulance delayed: hospital informed, bay prep cancelled", "EXTERNAL_EVENT + cancel() on the transport stage", [&] {
        Pieces p;
        Stage *t = buildEmergencyAdmission(*c.w, p);
        t->makeAvailable();
        t->activateChildren();
        const int before = c.w->notifier.getNotificationCount();
        WorkflowEvent delayed(EventType::EXTERNAL_EVENT, "ambulance-12", stamp(),
                              "Ambulance 12 delayed: road closure");
        c.w->bus.publish(&delayed);
        check(c.w->notifier.getNotificationCount() == before + 1, "the hospital should be informed of the delay");
        p.transport->cancel();
        check(state(p.pickup) == "Cancelled", "pickup should be Cancelled, was " + qs(state(p.pickup)));
        check(state(p.transportT) == "Cancelled", "transport should be Cancelled");
        check(state(p.bay) == "Cancelled", "bay preparation should be Cancelled");
        check(p.transport->isFinished(), "a fully cancelled stage counts as finished");
        delete t;
    });

    theatre.step("Patient deteriorates during intake: escalated", "Intake is rejected and handed to the doctor", [&] {
        Pieces p;
        Stage *arr = makeArrivalStage(*c.w, p);
        arr->makeAvailable();
        p.regW->setData("patientName", "Unknown female, approx. 30");
        p.regW->setData("complaint", "Shortness of breath");
        drive(p.regW, &c.w->secretary);
        arr->activateChildren();
        drive(p.records, &c.w->secretary);
        arr->activateChildren();
        drive(p.handover, &c.w->paramedic);
        arr->activateChildren();
        check(released(p.intakeT), "intake should be released");

        p.intakeW->assign(&c.w->nurseKhoza);
        p.intakeW->start();
        p.intakeW->reject("Patient deteriorating: escalate to emergency treatment");
        check(state(p.intakeW) == "Rejected", "intake should be Rejected, was " + qs(state(p.intakeW)));
        check(p.intakeW->getData("rejectionReason").find("emergency treatment") != std::string::npos,
              "the escalation reason should be stored");
        check(p.intakeW->getAssignee() == 0, "the nurse should be released from the task");

        p.intakeW->makeAvailable();
        p.intakeW->assign(&c.w->drNaidoo);
        check(p.intakeW->getAssignee() == &c.w->drNaidoo, "the doctor should take over the patient");
        delete arr;
    });

    theatre.step("Patient does not survive: workflow ends as deceased", "cancel() the whole case; the outcome is recorded and archived", [&] {
        Pieces p;
        Stage *t = buildEmergencyAdmission(*c.w, p);
        t->makeAvailable();
        t->activateChildren();
        t->cancel();
        check(t->isFinished(), "a cancelled case should be finished");
        check(state(p.room) == "Cancelled", "unfinished work should be Cancelled");
        t->setData("outcome", "Deceased");
        check(t->getData("outcome") == "Deceased", "the outcome should be recorded as Deceased");
        WorkflowEvent done(EventType::WORKFLOW_FINISHED, "emergency-admission", stamp(),
                           "Patient deceased: history archived");
        c.w->bus.publish(&done);
        check(anyContains(c.w->audit.getRecords(), "Patient deceased"), "the audit log should archive the outcome");
        delete t;
    });

    // ------------------------------------------------------------------
    theatre.section("Hospital 13. End to end");

    theatre.step("Run a fresh emergency admission to completion", "Every task, in order, until the root finishes", [&] {
        Pieces p;
        Stage *t = buildEmergencyAdmission(*c.w, p);
        p.regW->setData("patientName", "Unknown male, approx. 70");
        p.regW->setData("complaint", "Fall with head injury");
        runToCompletion(theatre, t, &c.w->nurseKhoza);
        t->setData("outcome", "Admitted to ward");
        check(t->getData("outcome") == "Admitted to ward", "the outcome should be recorded");
        delete t;
    });
}

// ===========================================================================
// PART B - LEGAL
//
// Legal 1 to 5 are the original steps, unchanged: a small case built from plain
// tasks (Composite, lifecycle, parallel discovery, reject and cancel).
// Legal 6 onward add the detail: a full litigation matter with staff, roles,
// decorators, adapters, approvals, observers, undo, iterators and failure paths.
//
// The full matter:
//   Client intake (sequential)
//       Client interview (validated) -> conflict check ->
//       [engagement letter || trust deposit] (parallel)
//   Investigation (parallel)
//       evidence || legal research || witness statements ||
//       [expert brief -> expert report] (sequential, nested)
//   Pleadings and court (sequential)
//       Draft (audited) -> senior review (audited + priority) ->
//       e-file with the court (ExternalTask / adapter) -> attend hearing
//   Judgment and costs (sequential)
//       Record judgment -> agree costs / settlement (approval chain)
//   Billing and closure (sequential)
//       Invoice the client -> archive the case file
//
// External systems: court e-filing portal (APITwo via AdapterTwo) and the
// firm's case records (APIOne via AdapterOne).
// ===========================================================================

// Everything the firm needs that is NOT the matter tree itself.
struct LegalWorld {
    // roles (the number is the authority level used by the approval chain)
    Role paralegalRole, associateRole, seniorRole, partnerRole, managingRole, financeRole;

    // staff
    Participant paralegalSipho, paralegalAnke, associateZanele, associateJohan,
                seniorMokoena, partnerPillay, managingVanWyk, financeNomsa;

    // Strategy: who gets the next task
    RoleBasedAssignment byRole;
    WorkloadAssignment byWorkload;
    RuleBasedAssignment lightLoadAssociate;   // an associate with fewer than 2 open matters

    // Observer: one bus, three independent listeners
    EventPublisher bus;
    AuditLogger audit;
    Notifier notifier;
    MetricsCollector metrics;

    // Adapter: the two external systems the firm talks to
    APIOne caseRecords;
    APITwo courtPortal;
    AdapterOne recordsAdapter;
    AdapterTwo courtAdapter;

    // Chain of Responsibility: Senior Attorney -> Partner -> Managing Partner.
    // The head of the chain owns the rest, so only 'chain' is ever deleted.
    ApprovalLevel *seniorLevel, *partnerLevel, *managingLevel, *chain;

    LegalWorld()
        : paralegalRole("Paralegal", 1), associateRole("Associate", 2),
          seniorRole("Senior Attorney", 3), partnerRole("Partner", 4),
          managingRole("Managing Partner", 5), financeRole("Finance Clerk", 1),
          paralegalSipho("PL1", "Paralegal Sipho", &paralegalRole),
          paralegalAnke("PL2", "Paralegal Anke", &paralegalRole),
          associateZanele("AS1", "Associate Zanele", &associateRole),
          associateJohan("AS2", "Associate Johan", &associateRole),
          seniorMokoena("SA1", "Senior Attorney Mokoena", &seniorRole),
          partnerPillay("PT1", "Partner Pillay", &partnerRole),
          managingVanWyk("MP1", "Managing Partner van Wyk", &managingRole),
          financeNomsa("FIN1", "Finance Clerk Nomsa", &financeRole),
          lightLoadAssociate([this](Task*, Participant *p) {
              return p->hasRole(&associateRole) && p->getWorkload() < 2;
          }),
          recordsAdapter(&caseRecords),
          courtAdapter(&courtPortal) {
        seniorLevel   = new ApprovalLevel(&seniorRole, 1800);
        partnerLevel  = new ApprovalLevel(&partnerRole, 1800);
        managingLevel = new ApprovalLevel(&managingRole, 3600);
        seniorLevel->setNext(partnerLevel);
        partnerLevel->setNext(managingLevel);
        chain = seniorLevel;

        bus.attach(&audit);
        bus.attach(&notifier);
        bus.attach(&metrics);
    }
    ~LegalWorld() { delete chain; }

    std::vector<Participant*> staff() {
        std::vector<Participant*> v;
        v.push_back(&paralegalSipho);  v.push_back(&paralegalAnke);
        v.push_back(&associateZanele); v.push_back(&associateJohan);
        v.push_back(&seniorMokoena);   v.push_back(&partnerPillay);
        v.push_back(&managingVanWyk);  v.push_back(&financeNomsa);
        return v;
    }
};

// Raw task pointers, so a step can reach a task without searching the tree.
// 'xxxW' is the item as it sits in the tree (possibly wrapped in decorators).
struct LegalPieces {
    Stage *intake = 0, *onboarding = 0, *investigation = 0, *expertRun = 0,
          *pleadings = 0, *judgment = 0, *billing = 0;
    ExternalTask *efile = 0;
    Task *clientT = 0, *conflict = 0, *engagement = 0, *trust = 0,
         *evidence = 0, *research = 0, *witness = 0, *expertBrief = 0, *expertReport = 0,
         *draftT = 0, *reviewT = 0, *hearing = 0,
         *judgmentT = 0, *settle = 0, *invoice = 0, *archive = 0;
    WorkItem *clientW = 0, *draftW = 0, *reviewW = 0;
};

// ---- the five stages of the litigation matter --------------------------------

// Interview (validated), conflict check, then engagement + trust deposit in parallel.
Stage *makeLegalIntakeStage(LegalWorld &w, LegalPieces &p) {
    p.intake = new SequentialStage("intake-stage", "Client intake and conflict check");

    p.clientT = new Task("client", "Client intake interview", &w.paralegalRole, &w.byRole);
    std::vector<std::string> required;
    required.push_back("clientName");
    required.push_back("matter");
    p.clientW = new ValidatedTask(p.clientT, required);          // Decorator: needs client data

    p.conflict = new Task("conflict", "Conflict of interest check", &w.associateRole, &w.byWorkload);

    p.onboarding = new ParallelStage("onboarding", "Engagement and trust deposit");
    p.engagement = new Task("engagement", "Send engagement letter", &w.associateRole, &w.byWorkload);
    p.trust = new Task("trust", "Receive trust deposit", &w.financeRole, &w.byRole);
    p.onboarding->add(p.engagement);
    p.onboarding->add(p.trust);

    p.intake->add(p.clientW);
    p.intake->add(p.conflict);
    p.intake->add(p.onboarding);
    return p.intake;
}

// Four lines of investigation run side by side; the expert report waits for the brief.
Stage *makeLegalInvestigationStage(LegalWorld &w, LegalPieces &p) {
    p.investigation = new ParallelStage("investigation-stage", "Investigate the case");
    p.evidence = new Task("evidence", "Gather evidence", &w.paralegalRole, &w.byRole);
    p.research = new Task("research", "Legal research and precedents", &w.associateRole, &w.lightLoadAssociate);
    p.witness = new Task("witness", "Interview witnesses", &w.paralegalRole, &w.byRole);

    p.expertRun = new SequentialStage("expert-run", "Expert witness");
    p.expertBrief = new Task("expert-brief", "Brief the expert witness", &w.associateRole, &w.byWorkload);
    p.expertReport = new Task("expert-report", "Receive expert report", &w.associateRole, &w.byWorkload);
    p.expertRun->add(p.expertBrief);
    p.expertRun->add(p.expertReport);

    p.investigation->add(p.evidence);
    p.investigation->add(p.research);
    p.investigation->add(p.witness);
    p.investigation->add(p.expertRun);
    return p.investigation;
}

// Draft (audited), senior review (audited + priority), e-file (adapter), hearing.
Stage *makeLegalPleadingsStage(LegalWorld &w, LegalPieces &p) {
    p.pleadings = new SequentialStage("pleadings-stage", "Pleadings and court");

    p.draftT = new Task("draft", "Draft pleadings", &w.associateRole, &w.lightLoadAssociate);
    p.draftW = new AuditedTask(p.draftT, &w.bus);                // Decorator: audited

    p.reviewT = new Task("review", "Senior review and sign-off", &w.seniorRole, &w.byRole);
    WorkItem *audited = new AuditedTask(p.reviewT, &w.bus);      // Decorator: audited ...
    p.reviewW = new PriorityTask(audited, 2);                    // ... and stacked with priority 2 = high

    p.efile = new ExternalTask("efile", "E-file pleadings with the court (portal notified)",
                               &w.paralegalRole, &w.byRole, &w.courtAdapter, "dispatch");
    p.hearing = new Task("hearing", "Attend hearing", &w.seniorRole, &w.byRole);

    p.pleadings->add(p.draftW);
    p.pleadings->add(p.reviewW);
    p.pleadings->add(p.efile);
    p.pleadings->add(p.hearing);
    return p.pleadings;
}

// Judgment, then costs / settlement (which needs approval).
Stage *makeLegalJudgmentStage(LegalWorld &w, LegalPieces &p) {
    p.judgment = new SequentialStage("judgment-stage", "Judgment and costs");
    p.judgmentT = new Task("judgment", "Record judgment", &w.seniorRole, &w.byRole);
    p.settle = new Task("settle", "Agree costs / settlement", &w.partnerRole, &w.byRole);
    p.judgment->add(p.judgmentT);
    p.judgment->add(p.settle);
    return p.judgment;
}

// Invoice only after costs are agreed; the file is archived last.
Stage *makeLegalBillingStage(LegalWorld &w, LegalPieces &p) {
    p.billing = new SequentialStage("billing-stage", "Billing and closure");
    p.invoice = new Task("invoice", "Invoice the client", &w.financeRole, &w.byRole);
    p.archive = new Task("archive-case", "Archive the case file", &w.paralegalRole, &w.byRole);
    p.billing->add(p.invoice);
    p.billing->add(p.archive);
    return p.billing;
}

Stage *buildLegalCase(LegalWorld &w, LegalPieces &p) {
    Stage *root = new SequentialStage("legal-case", "Litigation matter");
    root->add(makeLegalIntakeStage(w, p));
    root->add(makeLegalInvestigationStage(w, p));
    root->add(makeLegalPleadingsStage(w, p));
    root->add(makeLegalJudgmentStage(w, p));
    root->add(makeLegalBillingStage(w, p));
    return root;
}

struct LegalCtx {
    Participant who;
    Stage *root, *intake, *discovery, *litigation;
    Task *client, *conflict, *evidence, *research, *witness,
         *draft, *review, *filing, *hearing, *judgment, *invoice, *archive;

    LegalCtx() : who("lawyer", "Test Attorney", 0),
        root(0), intake(0), discovery(0), litigation(0),
        client(0), conflict(0), evidence(0), research(0), witness(0),
        draft(0), review(0), filing(0), hearing(0), judgment(0),
        invoice(0), archive(0) {}

    void reset() {
        delete root;
        root = intake = discovery = litigation = 0;
        client = conflict = evidence = research = witness = 0;
        draft = review = filing = hearing = judgment = invoice = archive = 0;
    }
    ~LegalCtx() { reset(); }
};

// Build exactly the workflow described in the legal demonstration:
// intake (sequential) -> discovery (parallel) -> litigation (sequential).
void buildLegalFull(LegalCtx &c) {
    c.reset();
    c.root = new SequentialStage("legal-case", "Legal Case Workflow");

    c.intake = new SequentialStage("intake-stage", "Client intake");
    c.client = new Task("client", "Paralegal conducts the client's intake interview", 0, 0);
    c.conflict = new Task("conflict", "Associate checks the firm for conflicts involving the opposing party", 0, 0);
    c.intake->add(c.client);
    c.intake->add(c.conflict);

    c.discovery = new ParallelStage("discovery-stage", "Parallel discovery");
    c.evidence = new Task("evidence", "Paralegal gathers documents and evidence for the client's case", 0, 0);
    c.research = new Task("research", "Associate researches the law and precedents relevant to the client's case", 0, 0);
    c.witness = new Task("witness", "Paralegal interviews witnesses connected to the client and the opposing party", 0, 0);
    c.discovery->add(c.evidence);
    c.discovery->add(c.research);
    c.discovery->add(c.witness);

    c.litigation = new SequentialStage("litigation-stage", "Litigation");
    c.draft = new Task("draft", "Associate drafts court pleadings for the client's case against the opposing party", 0, 0);
    c.review = new Task("review", "Partner reviews the client's pleadings and decides whether to approve them", 0, 0);
    c.filing = new Task("filing", "Paralegal files the approved pleadings with the court", 0, 0);
    c.hearing = new Task("hearing", "Legal representative attends the hearing for the client's case", 0, 0);
    c.judgment = new Task("judgment", "Senior attorney records the court's judgment in the client's case", 0, 0);
    c.invoice = new Task("invoice", "Finance clerk invoices the client for the legal work", 0, 0);
    c.archive = new Task("archive", "Paralegal archives the completed legal case file", 0, 0);
    c.litigation->add(c.draft);
    c.litigation->add(c.review);
    c.litigation->add(c.filing);
    c.litigation->add(c.hearing);
    c.litigation->add(c.judgment);
    c.litigation->add(c.invoice);
    c.litigation->add(c.archive);

    c.root->add(c.intake);
    c.root->add(c.discovery);
    c.root->add(c.litigation);
}

void addLegalScenario(Desktop &theatre, LegalCtx &c) {
    auto &check = Desktop::check;
    auto &expectThrows = Desktop::expectThrows;

    // ------------------------------------------------------------------
    theatre.section("Legal 1. Build the legal case workflow (Composite)");

    theatre.step("Open the legal case", "Create the root SequentialStage for the Legal Case Workflow", [&] {
        c.reset();
        c.root = new SequentialStage("legal-case", "Legal Case Workflow");
        check(c.root->getChildren().empty(), "the new root should be empty");
        theatre.say("Legal case opened | Root: Legal Case Workflow");
    });

    theatre.step("Add the client's intake and conflict check", "Sequential: paralegal interviews the client -> associate checks for conflicts involving the opposing party", [&] {
        c.intake = new SequentialStage("intake-stage", "Client intake");
        c.client = new Task("client", "Paralegal conducts the client's intake interview", 0, 0);
        c.conflict = new Task("conflict", "Associate checks the firm for conflicts involving the opposing party", 0, 0);
        c.intake->add(c.client);
        c.intake->add(c.conflict);
        c.root->add(c.intake);
        check(c.intake->getChildren().size() == 2, "intake should contain two tasks");
        theatre.say("Legal Case Workflow\n  Sequential: client intake and conflict check\n    1. Paralegal interviews the client and records the matter details\n    2. Associate checks whether the firm has a conflict involving the opposing party");
    });

    theatre.step("Add the parallel investigation into the client's case", "Three branches run together: collect case evidence || research relevant law and precedents || interview witnesses", [&] {
        c.discovery = new ParallelStage("discovery-stage", "Parallel discovery");
        c.evidence = new Task("evidence", "Paralegal gathers documents and evidence for the client's case", 0, 0);
        c.research = new Task("research", "Associate researches the law and precedents relevant to the client's case", 0, 0);
        c.witness = new Task("witness", "Paralegal interviews witnesses connected to the client and the opposing party", 0, 0);
        c.discovery->add(c.evidence);
        c.discovery->add(c.research);
        c.discovery->add(c.witness);
        c.root->add(c.discovery);
        check(c.discovery->getChildren().size() == 3, "discovery should contain three parallel tasks");
        theatre.say("  Parallel investigation into the client's case\n    - Gather documents and supporting evidence\n    - Research applicable law and court precedents\n    - Interview witnesses connected to the dispute");
    });

    theatre.step("Add pleadings, court and case closeout", "Sequential: associate drafts the client's pleadings -> partner reviews -> file with court -> hearing -> judgment -> invoice the client -> archive case", [&] {
        c.litigation = new SequentialStage("litigation-stage", "Litigation");
        c.draft = new Task("draft", "Associate drafts court pleadings for the client's case against the opposing party", 0, 0);
        c.review = new Task("review", "Partner reviews the client's pleadings and decides whether to approve them", 0, 0);
        c.filing = new Task("filing", "Paralegal files the approved pleadings with the court", 0, 0);
        c.hearing = new Task("hearing", "Legal representative attends the hearing for the client's case", 0, 0);
        c.judgment = new Task("judgment", "Senior attorney records the court's judgment in the client's case", 0, 0);
        c.invoice = new Task("invoice", "Finance clerk invoices the client for the legal work", 0, 0);
        c.archive = new Task("archive", "Paralegal archives the completed legal case file", 0, 0);
        c.litigation->add(c.draft);
        c.litigation->add(c.review);
        c.litigation->add(c.filing);
        c.litigation->add(c.hearing);
        c.litigation->add(c.judgment);
        c.litigation->add(c.invoice);
        c.litigation->add(c.archive);
        c.root->add(c.litigation);
        check(c.root->getChildren().size() == 3, "root should contain intake, discovery and litigation");
        check(c.litigation->getChildren().size() == 7, "litigation should contain seven sequential tasks");
        theatre.say("Legal case structure complete: 3 main stages, 12 tasks");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 2. Legal case: initial state and sequential gating");

    theatre.step("All work on the client's case starts as Created", "The intake, investigation and court tasks are waiting; none can begin before the case is opened", [&] {
        check(state(c.client) == "Created", "client interview should start Created");
        check(state(c.conflict) == "Created", "conflict check should start Created");
        check(state(c.evidence) == "Created", "evidence should start Created");
        check(state(c.draft) == "Created", "drafting should start Created");
        theatre.say("Legal case: all 12 tasks are Created");
    });

    theatre.step("Open the client's case file", "Only the paralegal's intake interview with the client is released; the conflict check waits", [&] {
        c.root->makeAvailable();
        c.root->activateChildren();
        check(released(c.client), "client intake should be released");
        check(!released(c.conflict), "conflict check must wait for intake");
        check(!released(c.evidence), "discovery must wait for intake");
        check(!released(c.draft), "drafting must wait for discovery");
        sayAvailable(theatre, c.root);
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 3. Client intake interview: task lifecycle (State)");

    theatre.step("Assign the client's intake interview", "The available client interview is assigned to the attorney handling the client's matter: Available -> Assigned", [&] {
        c.client->assign(&c.who);
        check(state(c.client) == "Assigned", "client interview should be Assigned");
    });

    theatre.step("Begin the interview with the client", "The assigned attorney starts recording the client's instructions: Assigned -> Started", [&] {
        c.client->start();
        check(state(c.client) == "Started", "client interview should be Started");
    });

    theatre.step("Finish the client's intake interview", "Started -> Completed; the associate can now run the conflict check involving the opposing party", [&] {
        c.client->complete();
        check(state(c.client) == "Completed", "client interview should be Completed");
        c.root->activateChildren();
        check(released(c.conflict), "conflict check should be released after intake interview");
        sayAvailable(theatre, c.root);
    });

    theatre.step("Prevent the completed interview from being restarted", "the client's completed intake interview cannot move back to Started", [&] {
        expectThrows([&] { c.client->start(); }, "start() on a completed client interview");
        check(state(c.client) == "Completed", "the task must remain Completed");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 4. Investigate the client's case in parallel");

    theatre.step("Clear the conflict check involving the opposing party", "Once the associate confirms the firm can act, evidence gathering, legal research and witness interviews all open together", [&] {
        drive(c.conflict, &c.who);
        c.root->activateChildren();
        check(released(c.evidence), "evidence should be released");
        check(released(c.research), "research should be released");
        check(released(c.witness), "witness interviews should be released");
        check(availableNow(c.root).size() == 3, "all three discovery tasks should be available");
        sayAvailable(theatre, c.root);
    });

    theatre.step("Collect the client's supporting evidence", "The evidence branch completes, but the investigation stays open while legal research and witness interviews continue", [&] {
        drive(c.evidence, &c.who);
        c.root->activateChildren();
        check(c.evidence->isFinished(), "evidence should be completed");
        check(!c.discovery->isFinished(), "parallel discovery must still be active");
        check(released(c.research) && released(c.witness), "the other discovery tasks should remain available");
        sayAvailable(theatre, c.root);
    });

    theatre.step("Finish legal research and witness interviews", "The investigation closes only after the law and precedents have been researched and all relevant witnesses interviewed", [&] {
        drive(c.research, &c.who);
        drive(c.witness, &c.who);
        c.root->activateChildren();
        check(c.discovery->isFinished(), "discovery should be finished");
        check(c.intake->isFinished(), "the completed intake/discovery sequence should be finished");
        check(released(c.draft), "drafting should now be released");
        sayAvailable(theatre, c.root);
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 5. Partner rejection and court-filing cancellation");

    theatre.step("Prepare the pleadings for the client's case against the opposing party", "The associate can draft court papers only after the client's intake and all investigation branches are complete", [&] {
        check(released(c.draft), "drafting should be available");
        drive(c.draft, &c.who);
        c.root->activateChildren();
        check(c.draft->isFinished(), "drafting should be completed");
        check(released(c.review), "partner review should be released after drafting");
    });

    theatre.step("Partner returns the client's pleadings for revision", "The partner rejects the draft and records the reason: Pleadings need revision", [&] {
        c.review->assign(&c.who);
        c.review->start();
        c.review->reject("Pleadings need revision");
        check(state(c.review) == "Rejected", "review should be Rejected");
        check(c.review->getData("rejectionReason") == "Pleadings need revision",
              "the rejection reason should be stored");
        check(c.review->getAssignee() == 0, "the partner should be released from the rejected task");
        theatre.say("Legal case | Partner review: Rejected\nReason: Pleadings need revision");
    });

    theatre.step("Cancel the court filing for the client's case", "The filing task is cancelled and moves to Cancelled before the papers are submitted", [&] {
        c.filing->cancel();
        check(state(c.filing) == "Cancelled", "filing should be Cancelled");
        theatre.say("Legal case | Court filing: Cancelled");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 6. End-to-end: the legal case");

    theatre.step("Complete the legal case from intake to archive", "Interview the client -> conflict check involving the opposing party -> evidence, research and witnesses -> pleadings -> partner approval -> court filing -> hearing -> judgment -> invoice -> archive", [&] {
        LegalCtx full;
        buildLegalFull(full);
        runToCompletion(theatre, full.root, &full.who);
        check(full.root->isFinished(), "the legal case root should finish");
        check(state(full.archive) == "Completed", "the final archive task should be Completed");
        theatre.say("Legal case completed: all 12 tasks Completed; root Legal Case Workflow Completed");
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