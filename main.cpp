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
    // original small case (Legal 1 to 5)
    Participant who;
    Stage *root = 0, *intake = 0, *discovery = 0, *litigation = 0;
    Task *client = 0, *conflict = 0, *evidence = 0, *research = 0, *draft = 0, *review = 0, *filing = 0;

    // full litigation matter (Legal 6 onward)
    LegalWorld *w = 0;
    Stage *full = 0;           // matter A, the case we walk through step by step
    WorkItem *matterB = 0;     // a second matter cloned from the same definition
    LegalPieces p;

    LegalCtx() : who("lawyer", "Test Attorney", 0) {}

    void resetSmall() {
        delete root;
        root = intake = discovery = litigation = 0;
        client = conflict = evidence = research = draft = review = filing = 0;
    }
    // The tree is always deleted before the world it points into.
    void resetFirm() {
        delete matterB;
        delete full;
        delete w;
        matterB = 0;
        full = 0;
        w = 0;
        p = LegalPieces();
    }
    void reset() { resetFirm(); resetSmall(); }
    ~LegalCtx() { reset(); }
};

// Full legal case tree in one go, used by the original end-to-end step.
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

    // ==================================================================
    // ORIGINAL STEPS (unchanged)
    // ==================================================================

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
        sayAvailable(theatre, c.root);
        check(availableNow(c.root).size() == 2, "the iterator should find evidence and research");
    });

    theatre.step("One discovery task done, stage not done", "Stage waits for every child", [&] {
        drive(c.evidence, &c.who);
        c.root->activateChildren();
        check(c.evidence->isFinished(), "evidence gathering should be finished");
        check(!c.discovery->isFinished(), "discovery must not finish while research is open");
        sayAvailable(theatre, c.root);
        check(availableNow(c.root).size() == 1, "only research should still be available");
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

    // ==================================================================
    // ADDED DETAIL: the full litigation matter
    // ==================================================================

    // ------------------------------------------------------------------
    theatre.section("Legal 6. Staff, roles and assignment (Strategy)");

    theatre.step("Create the law firm", "Paralegals, 2 associates, senior attorney, partner, managing partner, finance clerk", [&] {
        c.resetFirm();
        c.w = new LegalWorld();
        check(c.w->paralegalSipho.hasRole(&c.w->paralegalRole), "Sipho should hold the Paralegal role");
        check(c.w->associateRole.getAuthorityLevel() < c.w->seniorRole.getAuthorityLevel(),
              "a senior attorney outranks an associate");
        check(c.w->seniorRole.getAuthorityLevel() < c.w->partnerRole.getAuthorityLevel(),
              "a partner outranks a senior attorney");
        check(c.w->partnerRole.getAuthorityLevel() < c.w->managingRole.getAuthorityLevel(),
              "the Managing Partner outranks a partner");
        theatre.say("8 staff, 6 roles, approval authority 1 to 5");
    });

    theatre.step("Role-based: a research task goes to an associate", "RoleBasedAssignment skips paralegals, finance and partners", [&] {
        Task probe("probe", "Probe associate task", &c.w->associateRole, &c.w->byRole);
        Participant *who = c.w->byRole.selectParticipant(&probe, c.w->staff());
        check(who != 0, "someone should be selected");
        check(who->hasRole(&c.w->associateRole), "selected " + qs(who->getName()) + " is not an associate");
    });

    theatre.step("Workload-based: the least busy associate is chosen", "Associate Zanele has 3 open matters, Associate Johan has none", [&] {
        c.w->associateZanele.incrementWorkload();
        c.w->associateZanele.incrementWorkload();
        c.w->associateZanele.incrementWorkload();
        Task probe("probe", "Probe associate task", &c.w->associateRole, &c.w->byWorkload);
        Participant *who = c.w->byWorkload.selectParticipant(&probe, c.w->staff());
        check(who == &c.w->associateJohan, "expected Associate Johan, got " + qs(who ? who->getName() : "nobody"));
    });

    theatre.step("Rule-based: skip an associate with 2 open matters", "Rule: an associate with fewer than 2 open matters", [&] {
        Task probe("probe", "Probe associate task", &c.w->associateRole, &c.w->lightLoadAssociate);
        Participant *who = c.w->lightLoadAssociate.selectParticipant(&probe, c.w->staff());
        check(who == &c.w->associateJohan, "expected Associate Johan, got " + qs(who ? who->getName() : "nobody"));
        check(who != &c.w->associateZanele, "Zanele is over the limit and must be skipped");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 7. Build the litigation matter (Composite + Decorator)");

    theatre.step("Create the full matter's root stage", "Empty SequentialStage 'Litigation matter'", [&] {
        c.full = new SequentialStage("legal-case", "Litigation matter");
        check(c.full->getChildren().empty(), "A new stage should have no children");
    });

    theatre.step("Add client intake", "interview -> conflict check -> [engagement || trust deposit]", [&] {
        c.full->add(makeLegalIntakeStage(*c.w, c.p));
        check(c.p.intake->getChildren().size() == 3, "intake: interview + conflict check + onboarding");
        check(c.p.onboarding->getChildren().size() == 2, "onboarding: engagement + trust deposit");
    });

    theatre.step("Add the investigation", "ParallelStage: evidence || research || witnesses || [expert brief -> report]", [&] {
        c.full->add(makeLegalInvestigationStage(*c.w, c.p));
        check(c.p.investigation->getChildren().size() == 4, "investigation: 3 tasks + expert run");
        check(c.p.expertRun->getChildren().size() == 2, "expert run: brief + report");
    });

    theatre.step("Add pleadings and court", "draft -> senior review -> e-file -> hearing", [&] {
        c.full->add(makeLegalPleadingsStage(*c.w, c.p));
        check(c.p.pleadings->getChildren().size() == 4, "pleadings stage should hold 4 tasks");
    });

    theatre.step("Add judgment and costs", "record judgment -> agree costs / settlement", [&] {
        c.full->add(makeLegalJudgmentStage(*c.w, c.p));
        check(c.p.judgment->getChildren().size() == 2, "judgment stage should hold 2 tasks");
    });

    theatre.step("Add billing and closure", "invoice the client -> archive the case file", [&] {
        c.full->add(makeLegalBillingStage(*c.w, c.p));
        check(c.p.billing->getChildren().size() == 2, "billing stage should hold 2 tasks");
        check(c.full->getChildren().size() == 5, "root should hold 5 stages");
    });

    theatre.step("Decorators wrap tasks without new task types", "Interview is validated, draft is audited, review is audited + priority 2", [&] {
        check(dynamic_cast<ValidatedTask*>(c.p.clientW) != 0, "the interview should be a ValidatedTask");
        check(dynamic_cast<AuditedTask*>(c.p.draftW) != 0, "the draft should be audited");
        PriorityTask *pr = dynamic_cast<PriorityTask*>(c.p.reviewW);
        check(pr != 0, "the review should be a PriorityTask");
        check(pr->getPriority() == 2, "review priority should be 2 (high)");
        check(dynamic_cast<AuditedTask*>(pr->getWrapped()) != 0, "the review should also be audited (stacked)");
        check(c.p.clientW->getItemID() == "client", "a decorator keeps the task's id");
    });

    theatre.step("Count the work", "17 tasks across 5 stages", [&] {
        std::vector<WorkItem*> leaves;
        collectLeaves(c.full, leaves);
        check(leaves.size() == 17, QString("expected 17 tasks, found %1").arg(leaves.size()));
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 8. Two matters, one template (Prototype)");

    theatre.step("Clone the workflow for a second client", "root->clone() copies the whole tree: stages, tasks and decorators", [&] {
        c.matterB = c.full->clone();
        check(c.matterB != c.full, "the clone must be a different object");
        check(c.matterB->getChildren().size() == c.full->getChildren().size(), "same shape as the original");
        check(findItem(c.matterB, "client") != findItem(c.full, "client"), "every task must be its own copy");
    });

    theatre.step("Matter B moves, matter A does not", "Driving B's interview leaves A's interview in Created", [&] {
        c.matterB->makeAvailable();
        c.matterB->activateChildren();
        WorkItem *bClient = findItem(c.matterB, "client");
        bClient->setData("clientName", "Dube Family Trust");
        bClient->setData("matter", "Estate dispute");
        drive(bClient, &c.w->paralegalSipho);
        c.matterB->activateChildren();
        check(bClient->isFinished(), "B's interview should be finished");
        check(state(c.p.clientT) == "Created", "A's interview must still be Created, was " + qs(state(c.p.clientT)));
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 9. Client intake (Decorator + Strategy + Parallel stage)");

    theatre.step("Open matter A: only the interview is released", "Conflict check and onboarding wait for the interview (sequential)", [&] {
        c.full->makeAvailable();
        c.full->activateChildren();
        check(released(c.p.clientT), "the interview should be released first");
        check(!released(c.p.conflict), "the conflict check must wait for the interview");
        check(!released(c.p.engagement), "onboarding must wait for the conflict check");
        check(!released(c.p.evidence), "the investigation must wait for intake");
        sayAvailable(theatre, c.full);
        check(availableNow(c.full).size() == 1, "the iterator should find exactly the interview");
    });

    theatre.step("Interview refuses to complete without client data", "ValidatedTask throws until clientName and matter are set", [&] {
        c.p.clientW->assign(&c.w->paralegalSipho);
        c.p.clientW->start();
        expectThrows([&] { c.p.clientW->complete(); }, "complete() with no client data");
        check(state(c.p.clientW) == "Started", "a refused completion must leave the task Started");
    });

    theatre.step("Paralegal captures the client details", "Fill clientName and matter, then complete", [&] {
        c.p.clientW->setData("clientName", "Mokwena Trading (Pty) Ltd");
        c.p.clientW->setData("matter", "Breach of contract, R2.4m");
        c.p.clientW->complete();
        check(state(c.p.clientW) == "Completed", "expected Completed, was " + qs(state(c.p.clientW)));
        c.full->activateChildren();
        check(released(c.p.conflict), "the conflict check should be released after the interview");
        sayAvailable(theatre, c.full);
        check(availableNow(c.full).size() == 1, "only the conflict check should be available now");
    });

    theatre.step("Conflict check goes to the least busy associate", "Strategy skips Associate Zanele (3 open matters)", [&] {
        Participant *who = c.w->byWorkload.selectParticipant(c.p.conflict, c.w->staff());
        check(who != 0 && who->hasRole(&c.w->associateRole), "an associate should be selected");
        check(who != &c.w->associateZanele, "the busy associate must be skipped");
        theatre.say("assigned to " + qs(who->getName()));
        c.p.conflict->assign(who);
        c.p.conflict->start();
        c.p.conflict->complete();
        c.full->activateChildren();
        check(c.p.conflict->isFinished(), "the conflict check should be complete");
        check(released(c.p.engagement), "the engagement letter should be released");
        check(released(c.p.trust), "the trust deposit is released in parallel");
    });

    theatre.step("Engagement letter and trust deposit run in parallel", "The stage waits for the slower branch", [&] {
        sayAvailable(theatre, c.full);
        check(availableNow(c.full).size() == 2, "the iterator should find engagement and trust deposit");
        drive(c.p.engagement, &c.w->associateJohan);
        c.full->activateChildren();
        check(c.p.engagement->isFinished(), "the engagement letter should be sent");
        check(!c.p.onboarding->isFinished(), "onboarding must wait for the trust deposit");
        drive(c.p.trust, &c.w->financeNomsa);
        c.full->activateChildren();
        check(c.p.onboarding->isFinished(), "onboarding should be finished");
        check(c.p.intake->isFinished(), "the whole intake stage should be finished");
    });

    theatre.step("Matter is opened in the firm's case records (Adapter)", "AdapterOne -> APIOne: add matter-2026-118 to case records", [&] {
        std::string operation = "add";
        std::string payload = "matter-2026-118=Mokwena Trading breach of contract";
        c.w->recordsAdapter.request(&operation, &payload);   // throws if the adapter rejects it
        check(c.w->recordsAdapter.getName() == "API One", "records adapter should front APIOne");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 10. Investigation (Parallel stage with a nested sequence)");

    theatre.step("Four lines of work open together", "Expert report waits for the brief", [&] {
        check(released(c.p.evidence), "evidence gathering should be released");
        check(released(c.p.research), "legal research should be released");
        check(released(c.p.witness), "witness interviews should be released");
        check(released(c.p.expertBrief), "the expert brief should be released");
        check(!released(c.p.expertReport), "the expert report must wait for the brief (sequential)");
        sayAvailable(theatre, c.full);
        check(availableNow(c.full).size() == 4, "the iterator should find all four lines of work");
    });

    theatre.step("Expert is briefed, report opens", "Finishing 'expert-brief' releases 'expert-report'", [&] {
        drive(c.p.expertBrief, &c.w->associateJohan);
        c.full->activateChildren();
        check(c.p.expertBrief->isFinished(), "the expert should be briefed");
        check(released(c.p.expertReport), "the report should be released after the brief");
    });

    theatre.step("Paralegals gather evidence and witness statements", "Two branches done, the stage is not", [&] {
        drive(c.p.evidence, &c.w->paralegalSipho);
        drive(c.p.witness, &c.w->paralegalAnke);
        c.full->activateChildren();
        check(c.p.evidence->isFinished() && c.p.witness->isFinished(), "both paralegal tasks should be finished");
        check(!c.p.investigation->isFinished(), "the investigation must not finish while research and the report are open");
        sayAvailable(theatre, c.full);
        check(availableNow(c.full).size() == 2, "research and the expert report should still be available");
    });

    theatre.step("Research goes to a lightly loaded associate", "Rule-based assignment; Zanele is over the limit", [&] {
        Participant *who = c.w->lightLoadAssociate.selectParticipant(c.p.research, c.w->staff());
        check(who != 0 && who->hasRole(&c.w->associateRole), "an associate should be selected");
        check(who != &c.w->associateZanele, "an associate with 3 open matters must be skipped");
        c.p.research->assign(who);
        c.p.research->start();
        c.p.research->complete();
        c.full->activateChildren();
        check(c.p.research->isFinished(), "legal research should be complete");
        check(!c.p.investigation->isFinished(), "the stage still waits for the expert report");
    });

    theatre.step("Expert report is the slowest branch", "Report arrives, investigation finishes, drafting opens", [&] {
        drive(c.p.expertReport, &c.w->associateJohan);
        c.full->activateChildren();
        check(c.p.expertRun->isFinished(), "the expert run should be finished");
        check(c.p.investigation->isFinished(), "the investigation should be finished");
        check(released(c.p.draftT), "drafting should open once the investigation is done");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 11. Pleadings and review (Command + Memento + State)");

    theatre.step("Draft goes to a lightly loaded associate, audited", "assign, start and complete are each logged", [&] {
        const std::size_t before = c.w->audit.getRecords().size();
        Participant *who = c.w->lightLoadAssociate.selectParticipant(c.p.draftT, c.w->staff());
        check(who != 0 && who != &c.w->associateZanele, "a lightly loaded associate should be selected");
        c.p.draftW->assign(who);
        c.p.draftW->start();
        c.p.draftW->complete();
        check(state(c.p.draftW) == "Completed", "expected Completed, was " + qs(state(c.p.draftW)));
        check(c.w->audit.getRecords().size() >= before + 3, "assign, start and complete should each be logged");
        c.full->activateChildren();
        check(released(c.p.reviewT), "the senior review should be released after the draft");
        check(!released(c.p.efile), "e-filing must wait for the review");
    });

    theatre.step("Review is high priority and audited", "PriorityTask(AuditedTask(Task)): priority 2 stored on the item", [&] {
        PriorityTask *pr = dynamic_cast<PriorityTask*>(c.p.reviewW);
        check(pr != 0 && pr->getPriority() == 2, "the review should carry priority 2");
        check(c.p.reviewW->getData("priority") == "2", "priority should be stored as data");
    });

    theatre.step("Wrong reviewer assigned, then undone", "AssignCommand snapshots the task (Memento); undo() restores it", [&] {
        CommandInvoker invoker;
        invoker.executeCommand(new AssignCommand(c.p.reviewT, &c.w->managingVanWyk));
        check(state(c.p.reviewT) == "Assigned", "expected Assigned, was " + qs(state(c.p.reviewT)));
        check(invoker.canUndo(), "the assignment should be undoable");
        check(invoker.undoLast(), "undo should succeed");
        check(state(c.p.reviewT) == "Available", "undo should restore Available, was " + qs(state(c.p.reviewT)));
        check(c.p.reviewT->getAssignee() == 0, "undo should clear the assignee");
    });

    theatre.step("Senior attorney is assigned but does not respond", "Deadline passes, the firm is told via a TIMEOUT event", [&] {
        c.p.reviewT->assign(&c.w->seniorMokoena);
        c.p.reviewT->setDeadline(std::time(0) - 60);
        check(c.p.reviewT->getDeadline() < std::time(0), "the review deadline should have passed");
        WorkflowEvent timeout(EventType::TIMEOUT, "review", stamp(),
                              "Senior Attorney Mokoena did not review the pleadings before the deadline");
        c.w->bus.publish(&timeout);
        check(anyContains(c.w->notifier.getNotifications(), "Approval timeout"),
              "the notifier should report the timeout");
    });

    theatre.step("Review is escalated to the partner", "reject -> Rejected -> makeAvailable -> assigned to Partner Pillay", [&] {
        c.p.reviewT->reject("No response within the deadline");
        check(state(c.p.reviewT) == "Rejected", "expected Rejected, was " + qs(state(c.p.reviewT)));
        check(c.p.reviewT->getAssignee() == 0, "a rejected task should have no assignee");
        check(c.p.reviewT->getData("rejectionReason") == "No response within the deadline",
              "the rejection reason should be stored");
        c.p.reviewT->makeAvailable();
        check(state(c.p.reviewT) == "Available", "the review should be available again");
        c.p.reviewT->assign(&c.w->partnerPillay);
        check(c.p.reviewT->getAssignee() == &c.w->partnerPillay, "the partner should take over the review");
    });

    theatre.step("Partner signs off the pleadings", "Started -> Completed (audited); e-filing is released", [&] {
        c.p.reviewW->start();
        c.p.reviewW->complete();
        c.full->activateChildren();
        check(c.p.reviewT->isFinished(), "the review should be complete");
        check(anyContains(c.w->audit.getRecords(), "was completed"), "the audit log should show the sign-off");
        check(released(c.p.efile), "e-filing should be released after sign-off");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 12. Settlement and cost approvals (Chain of Responsibility)");

    theatre.step("Small settlement: the senior attorney can approve", "Required level 3, senior attorney authority 3", [&] {
        ApprovalRequest request(c.p.settle, 3, std::time(0) + 1800);
        check(c.w->seniorLevel->canApprove(&request), "a senior attorney should approve a small settlement");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the request should be Approved");
    });

    theatre.step("Large settlement: escalates to the Partner", "Required level 4: senior attorney cannot approve, Partner can", [&] {
        ApprovalRequest request(c.p.settle, 4, std::time(0) + 1800);
        check(!c.w->seniorLevel->canApprove(&request), "a senior attorney must not approve a large settlement");
        check(c.w->partnerLevel->canApprove(&request), "the Partner can approve a large settlement");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the request should end up Approved");
    });

    theatre.step("Fee write-off: escalates to the Managing Partner", "Required level 5: only the top of the chain can approve", [&] {
        ApprovalRequest request(c.p.settle, 5, std::time(0) + 3600);
        check(!c.w->seniorLevel->canApprove(&request), "a senior attorney must not approve a write-off");
        check(!c.w->partnerLevel->canApprove(&request), "a Partner must not approve a write-off");
        check(c.w->managingLevel->canApprove(&request), "the Managing Partner can approve a write-off");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the request should end up Approved");
    });

    theatre.step("A rejected request stays rejected", "Once decided, later handlers cannot override it", [&] {
        ApprovalRequest request(c.p.settle, 4, std::time(0) + 1800);
        request.reject(&c.w->partnerRole);
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Rejected, "a rejected request must stay Rejected");
    });

    theatre.step("An unanswered approval expires and escalates", "Deadline passed -> isExpired -> the chain resets the deadline and decides", [&] {
        ApprovalRequest request(c.p.settle, 4, std::time(0) - 60);
        check(request.isExpired(std::time(0)), "an undecided request past its deadline is expired");
        c.w->chain->handle(&request);
        check(request.getStatus() == ApprovalStatus::Approved, "the escalated request should be Approved");
        check(!request.isExpired(std::time(0)), "a decided request is no longer expired");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 13. Court: e-filing and hearing (Adapter)");

    theatre.step("Paralegal starts the e-filing", "Assigned -> Started", [&] {
        c.p.efile->assign(&c.w->paralegalAnke);
        c.p.efile->start();
        check(state(c.p.efile) == "Started", "expected Started, was " + qs(state(c.p.efile)));
    });

    theatre.step("Court portal is notified (Adapter)", "ExternalTask -> AdapterTwo -> APITwo court portal", [&] {
        check(c.p.efile->execute(), "the filing request should be accepted");
        check(c.p.efile->getResponse() == "Request submitted to API Two",
              "unexpected portal response: " + qs(c.p.efile->getResponse()));
        check(c.w->courtAdapter.getName() == "API Two", "the court adapter should front APITwo");
        c.p.efile->complete();
        c.full->activateChildren();
        check(c.p.efile->isFinished(), "e-filing should be complete");
        check(released(c.p.hearing), "the hearing should be released after filing");
    });

    theatre.step("Senior attorney attends the hearing", "Role-based assignment, then straight to Completed", [&] {
        Participant *who = c.w->byRole.selectParticipant(c.p.hearing, c.w->staff());
        check(who != 0 && who->hasRole(&c.w->seniorRole), "a senior attorney should be selected");
        c.p.hearing->assign(who);
        c.p.hearing->start();
        c.p.hearing->complete();
        c.full->activateChildren();
        check(c.p.pleadings->isFinished(), "the pleadings stage should be finished");
        check(released(c.p.judgmentT), "recording the judgment should open after the hearing");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 14. Judgment, costs, billing and archive");

    theatre.step("Judgment is recorded", "Costs and settlement open", [&] {
        drive(c.p.judgmentT, &c.w->seniorMokoena);
        c.full->activateChildren();
        check(c.p.judgmentT->isFinished(), "the judgment should be recorded");
        check(released(c.p.settle), "costs / settlement should be released");
        check(!released(c.p.invoice), "invoicing must wait until costs are agreed");
    });

    theatre.step("Partner agrees the costs (approved)", "Invoicing opens", [&] {
        drive(c.p.settle, &c.w->partnerPillay);
        c.full->activateChildren();
        check(c.p.settle->isFinished(), "costs should be agreed");
        check(c.p.judgment->isFinished(), "the judgment stage should be finished");
        check(released(c.p.invoice), "invoicing should open once costs are agreed");
    });

    theatre.step("Finance invoices the client", "Created -> Completed", [&] {
        drive(c.p.invoice, &c.w->financeNomsa);
        c.full->activateChildren();
        check(c.p.invoice->isFinished(), "the invoice should be sent");
        check(released(c.p.archive), "archiving should open after invoicing");
    });

    theatre.step("Paralegal archives the case file", "Whole matter finishes: judgment for the client", [&] {
        drive(c.p.archive, &c.w->paralegalSipho);
        c.full->activateChildren();
        check(c.full->isFinished(), "the whole matter should be finished");
        c.full->setData("outcome", "Judgment for client, costs agreed");
        check(c.full->getData("outcome") == "Judgment for client, costs agreed", "the outcome should be recorded");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 15. Audit, notifications and metrics (Observer)");

    theatre.step("Audit log and notifier saw the same events", "One bus, independent listeners", [&] {
        const std::size_t audited = c.w->audit.getRecords().size();
        check(audited > 0, "the audit log should not be empty");
        check(static_cast<std::size_t>(c.w->notifier.getNotificationCount()) == audited,
              "every published event should reach both listeners");
        theatre.say(QString("%1 events on the bus").arg(audited));
    });

    theatre.step("Court portal reports an external event", "EXTERNAL_EVENT reaches the audit log and the notifier", [&] {
        const int before = c.w->notifier.getNotificationCount();
        WorkflowEvent external(EventType::EXTERNAL_EVENT, "court-portal", stamp(),
                               "Court portal: pleadings accepted, hearing set for 14 November");
        c.w->bus.publish(&external);
        check(c.w->notifier.getNotificationCount() == before + 1, "the notifier should receive the event");
        check(anyContains(c.w->notifier.getNotifications(), "External event received"),
              "the notification should name an external event");
        check(anyContains(c.w->audit.getRecords(), "hearing set for 14 November"),
              "the audit log should record the event details");
    });

    theatre.step("Metrics: how long was the hearing?", "Two state changes 45 minutes apart give an average of 2700 s", [&] {
        WorkflowEvent started(EventType::ITEM_STATE_CHANGED, "hearing", "2026-10-31 09:00:00", "Started");
        WorkflowEvent completed(EventType::ITEM_STATE_CHANGED, "hearing", "2026-10-31 09:45:00", "Completed");
        c.w->bus.publish(&started);
        c.w->bus.publish(&completed);
        check(c.w->metrics.getTransitionCount("Started") == 1, "one transition into Started");
        const float avg = c.w->metrics.getAverageTimeInState("Started");
        check(avg > 2699.9f && avg < 2700.1f, QString("expected 2700 s in Started, got %1").arg(avg));
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 16. Undo history (Command + Memento)");

    theatre.step("Matter B's conflict check: assign, start, complete", "Three commands, three snapshots, then three undos", [&] {
        WorkItem *bConflict = findItem(c.matterB, "conflict");
        check(released(bConflict), "B's conflict check should be released");
        CommandInvoker invoker;
        invoker.executeCommand(new AssignCommand(bConflict, &c.w->associateJohan));
        invoker.executeCommand(new StartCommand(bConflict));
        invoker.executeCommand(new CompleteCommand(bConflict));
        check(state(bConflict) == "Completed", "expected Completed, was " + qs(state(bConflict)));
        check(invoker.getHistory()->size() == 3, "three commands should be in the history");

        check(invoker.undoLast(), "first undo");
        check(state(bConflict) == "Started", "undoing Complete should give Started, was " + qs(state(bConflict)));
        check(invoker.undoLast(), "second undo");
        check(state(bConflict) == "Assigned", "undoing Start should give Assigned, was " + qs(state(bConflict)));
        check(invoker.undoLast(), "third undo");
        check(state(bConflict) == "Available", "undoing Assign should give Available, was " + qs(state(bConflict)));
        check(!invoker.canUndo(), "the history should now be empty");
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 17. Work moving from Created to Completed (Iterator)");

    theatre.step("Walk the whole tree (DepthFirstIterator)", "25 nodes: 8 stages + 17 tasks, every task still Created", [&] {
        LegalPieces p;
        Stage *t = buildLegalCase(*c.w, p);
        check(countNodes(t) == 25, QString("expected 25 nodes, walked %1").arg(countNodes(t)));
        theatre.say("tally: " + qs(stateTally(t)));
        check(stateTally(t) == "Created 17", "every task should start Created, was " + qs(stateTally(t)));
        delete t;
    });

    theatre.step("Drain the work with AvailableItemsIterator", "Ask the iterator what is available, complete one task, ask again, until nothing is left", [&] {
        LegalPieces p;
        Stage *t = buildLegalCase(*c.w, p);
        p.clientW->setData("clientName", "Khumalo & Sons");
        p.clientW->setData("matter", "Lease dispute");
        t->makeAvailable();

        std::vector<std::size_t> sizes;
        int passes = 0;
        for (; passes < 60; ++passes) {
            std::vector<WorkItem*> avail = availableNow(t);
            if (avail.empty()) break;
            sizes.push_back(avail.size());
            theatre.say(QString("pass %1: %2 available [%3] | %4")
                            .arg(passes + 1).arg(avail.size()).arg(qs(idList(avail))).arg(qs(stateTally(t))));
            drive(avail[0], &c.w->associateJohan);       // first available task goes Available -> Completed
            t->activateChildren();                        // next tasks are released
        }
        theatre.say("done: " + qs(stateTally(t)));
        check(passes == 17, QString("expected 17 passes (one per task), took %1").arg(passes));
        check(sizes.front() == 1, "the matter opens with only the client interview");
        check(*std::max_element(sizes.begin(), sizes.end()) == 4, "the investigation opens 4 tasks at once");
        check(sizes.back() == 1, "the last task is the only one left");
        check(availableNow(t).empty(), "nothing should be available at the end");
        check(stateTally(t) == "Completed 17", "every task should be Completed, was " + qs(stateTally(t)));
        check(t->isFinished(), "the root should be finished");
        delete t;
    });

    theatre.step("Matter A is done, matter B still has work", "The iterator sees each clone's own state (Prototype)", [&] {
        check(availableNow(c.full).empty(), "finished matter A should have nothing available");
        check(stateTally(c.full) == "Completed 17", "matter A should be Completed 17, was " + qs(stateTally(c.full)));
        c.matterB->activateChildren();
        sayAvailable(theatre, c.matterB);
        check(availableNow(c.matterB).size() == 1, "matter B should have its conflict check available");
        theatre.say("matter B: " + qs(stateTally(c.matterB)));
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 18. Failure paths");

    theatre.step("Conflict of interest found: matter declined", "Event published, cancel() on the whole matter, outcome recorded", [&] {
        LegalPieces p;
        Stage *t = buildLegalCase(*c.w, p);
        p.clientW->setData("clientName", "Ndlovu Logistics");
        p.clientW->setData("matter", "Contract claim against existing client");
        t->makeAvailable();
        t->activateChildren();
        drive(p.clientW, &c.w->paralegalSipho);
        t->activateChildren();
        check(released(p.conflict), "the conflict check should be released");
        const int before = c.w->notifier.getNotificationCount();
        WorkflowEvent conflict(EventType::EXTERNAL_EVENT, "conflict", stamp(),
                               "Conflict of interest: the firm already acts for the other party");
        c.w->bus.publish(&conflict);
        check(c.w->notifier.getNotificationCount() == before + 1, "the partner should be told about the conflict");
        t->cancel();
        check(t->isFinished(), "a declined matter should be finished");
        check(state(p.conflict) == "Cancelled", "the conflict check should be Cancelled, was " + qs(state(p.conflict)));
        check(state(p.draftT) == "Cancelled", "unstarted work should be Cancelled");
        check(state(p.clientW) == "Completed", "the interview that already happened stays Completed");
        t->setData("outcome", "Declined: conflict of interest");
        check(t->getData("outcome") == "Declined: conflict of interest", "the outcome should be recorded");
        delete t;
    });

    theatre.step("Client withdraws during the investigation", "cancel() stops every open task; finished work stays finished", [&] {
        LegalPieces p;
        Stage *t = buildLegalCase(*c.w, p);
        p.clientW->setData("clientName", "Botha Farms");
        p.clientW->setData("matter", "Boundary dispute");
        t->makeAvailable();
        t->activateChildren();
        drive(p.clientW, &c.w->paralegalSipho);
        t->activateChildren();
        drive(p.conflict, &c.w->associateJohan);
        t->activateChildren();
        drive(p.engagement, &c.w->associateJohan);
        drive(p.trust, &c.w->financeNomsa);
        t->activateChildren();
        check(released(p.evidence), "the investigation should be open");
        drive(p.evidence, &c.w->paralegalSipho);
        t->activateChildren();

        WorkflowEvent withdrawn(EventType::EXTERNAL_EVENT, "client", stamp(),
                                "Botha Farms withdrew instructions");
        c.w->bus.publish(&withdrawn);
        t->cancel();
        check(t->isFinished(), "a withdrawn matter should be finished");
        check(state(p.evidence) == "Completed", "the evidence already gathered stays Completed");
        check(state(p.research) == "Cancelled", "open research should be Cancelled, was " + qs(state(p.research)));
        check(state(p.expertBrief) == "Cancelled", "the expert brief should be Cancelled");
        check(state(p.efile) == "Cancelled", "e-filing should be Cancelled");
        check(state(p.archive) == "Cancelled", "archiving should be Cancelled");
        theatre.say("tally: " + qs(stateTally(t)));
        delete t;
    });

    theatre.step("Partner rejects the pleadings: sent back", "Review is rejected, reason stored, reviewer released, pleadings redrafted", [&] {
        LegalPieces p;
        Stage *pl = makeLegalPleadingsStage(*c.w, p);
        pl->makeAvailable();
        pl->activateChildren();
        check(released(p.draftT), "drafting should be released");
        drive(p.draftW, &c.w->associateJohan);
        pl->activateChildren();
        check(released(p.reviewT), "the review should be released");

        p.reviewW->assign(&c.w->partnerPillay);
        p.reviewW->start();
        p.reviewW->reject("Pleadings must plead the alternative claim");
        check(state(p.reviewW) == "Rejected", "the review should be Rejected, was " + qs(state(p.reviewW)));
        check(p.reviewW->getData("rejectionReason").find("alternative claim") != std::string::npos,
              "the rejection reason should be stored");
        check(p.reviewW->getAssignee() == 0, "the partner should be released from the task");

        p.reviewW->makeAvailable();
        p.reviewW->assign(&c.w->seniorMokoena);
        check(p.reviewW->getAssignee() == &c.w->seniorMokoena, "the senior attorney should take the revised review");
        delete pl;
    });

    theatre.step("Matter settles before the hearing: hearing cancelled", "Portal event + cancel() on the hearing only", [&] {
        LegalPieces p;
        Stage *pl = makeLegalPleadingsStage(*c.w, p);
        pl->makeAvailable();
        pl->activateChildren();
        const int before = c.w->notifier.getNotificationCount();
        WorkflowEvent settled(EventType::EXTERNAL_EVENT, "opposing-counsel", stamp(),
                              "Opposing counsel accepted the settlement offer");
        c.w->bus.publish(&settled);
        check(c.w->notifier.getNotificationCount() == before + 1, "the firm should be told about the settlement");
        p.hearing->cancel();
        check(state(p.hearing) == "Cancelled", "the hearing should be Cancelled, was " + qs(state(p.hearing)));
        check(state(p.draftT) != "Cancelled", "the rest of the pleadings are untouched");
        delete pl;
    });

    // ------------------------------------------------------------------
    theatre.section("Legal 19. End to end");

    theatre.step("Run a fresh legal case to completion",
                 "Drives every task in order and checks the root finishes", [&] {
        LegalCtx full;
        buildLegalFull(full);
        runToCompletion(theatre, full.root, &full.who);
    });

    theatre.step("Run a fresh litigation matter to completion", "The full matter with staff, decorators and adapters, every task in order", [&] {
        LegalPieces p;
        Stage *t = buildLegalCase(*c.w, p);
        p.clientW->setData("clientName", "Pretorius Properties");
        p.clientW->setData("matter", "Eviction application");
        runToCompletion(theatre, t, &c.w->associateJohan);
        t->setData("outcome", "Settled out of court");
        check(t->getData("outcome") == "Settled out of court", "the outcome should be recorded");
        delete t;
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