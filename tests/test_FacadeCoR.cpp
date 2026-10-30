/**
 * @file test_facade_approval_chain.cpp
 * @brief CHAIN OF RESPONSIBILITY behaviour reached through TIVIDYFacade::requestApproval.
 *
 * GoF mapping:
 *  - Handler          : ApprovalHandler
 *  - ConcreteHandler  : ApprovalLevel (Role + timeout)
 *  - Request          : ApprovalRequest (required level taken from the item's
 *                       data key "requiredApprovalLevel", default 1)
 *
 * Test chain (unless stated): Nurse(authority 1) -> Doctor(authority 3).
 */
#include "third_party/doctest.h"
#include "third_party/TestSupport.h"

using namespace testsupport;

namespace {

/** Registers a one-task definition "appr" with the standard chain and returns a started instance. */
std::string approvalInstance(Fixture& f, const std::string& levelValue, bool setLevel = true) {
    WorkflowDefinition* def = makeSequentialDefinition("appr", &f.workerRole, 1);
    if (setLevel) tagTasks(def, "requiredApprovalLevel", levelValue);
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("appr");
    f.facade.startInstance(id);
    return id;
}

} // namespace

TEST_SUITE("Facade/CoR: routing by required level") {

TEST_CASE("default level (no data key) is 1 and is approved by the first handler") {
    Fixture f;
    std::string id = approvalInstance(f, "", /*setLevel=*/false);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("level 1 -> approved at the first level (Nurse)") {
    Fixture f;
    std::string id = approvalInstance(f, "1");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("level 2 -> Nurse (1) cannot approve, request is escalated and Doctor (3) approves") {
    Fixture f;
    std::string id = approvalInstance(f, "2");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("level exactly equal to a role's authority is approved (>=, not >)") {
    Fixture f;
    std::string id = approvalInstance(f, "3");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("level above every authority in the chain ends as Escalated (nobody could approve)") {
    Fixture f;
    std::string id = approvalInstance(f, "4");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Escalated);
}

TEST_CASE("a very high level is Escalated, never Approved") {
    Fixture f;
    std::string id = approvalInstance(f, "1000000");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Escalated);
}

TEST_CASE("level 0 and negative levels are approved by the first handler") {
    Fixture f;
    std::string id = approvalInstance(f, "0");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
    Fixture g;
    std::string id2 = approvalInstance(g, "-5");
    CHECK(g.facade.requestApproval(id2, "1") == ApprovalStatus::Approved);
}

TEST_CASE("a non-numeric level parses as 0 (atoi) and is therefore approved at once") {
    Fixture f;
    std::string id = approvalInstance(f, "high");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("an empty-string level parses as 0 and is approved") {
    Fixture f;
    std::string id = approvalInstance(f, "");   // key present, value empty
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("a level with a numeric prefix uses the prefix ('2abc' -> 2, ' 3' -> 3)") {
    Fixture f;
    std::string id = approvalInstance(f, "4abc");
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Escalated);
    Fixture g;
    std::string id2 = approvalInstance(g, " 3");
    CHECK(g.facade.requestApproval(id2, "1") == ApprovalStatus::Approved);
}

} // TEST_SUITE

TEST_SUITE("Facade/CoR: chain shapes") {

TEST_CASE("single-level chain that is strong enough approves") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("one", &f.workerRole, 1);
    def->setApprovalChain(new ApprovalLevel(&f.doctorRole, 60));
    tagTasks(def, "requiredApprovalLevel", "3");
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("one");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("single-level chain that is too weak ends Escalated") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("weak", &f.workerRole, 1);
    def->setApprovalChain(new ApprovalLevel(&f.nurseRole, 60));
    tagTasks(def, "requiredApprovalLevel", "2");
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("weak");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Escalated);
}

TEST_CASE("a handler with a null Role cannot approve and passes the request on") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("nullrole", &f.workerRole, 1);
    ApprovalLevel* head = new ApprovalLevel(nullptr, 60);
    head->setNext(new ApprovalLevel(&f.doctorRole, 60));
    def->setApprovalChain(head);
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("nullrole");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("a chain consisting only of a null-role handler ends Escalated") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("onlynull", &f.workerRole, 1);
    def->setApprovalChain(new ApprovalLevel(nullptr, 60));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("onlynull");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Escalated);
}

TEST_CASE("a three-level chain escalates through every level") {
    Fixture f;
    Role exec("exec", 9);
    WorkflowDefinition* def = makeSequentialDefinition("three", &f.workerRole, 1);
    ApprovalLevel* a = new ApprovalLevel(&f.nurseRole, 60);
    ApprovalLevel* b = new ApprovalLevel(&f.doctorRole, 60);
    ApprovalLevel* c = new ApprovalLevel(&exec, 60);
    a->setNext(b);
    b->setNext(c);
    def->setApprovalChain(a);
    tagTasks(def, "requiredApprovalLevel", "8");
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("three");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("a strong head handler short-circuits the chain") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("strongfirst", &f.workerRole, 1);
    ApprovalLevel* head = new ApprovalLevel(&f.doctorRole, 60);
    head->setNext(new ApprovalLevel(&f.nurseRole, 60));
    def->setApprovalChain(head);
    tagTasks(def, "requiredApprovalLevel", "3");
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("strongfirst");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

} // TEST_SUITE

TEST_SUITE("Facade/CoR: error handling") {

TEST_CASE("no approval chain configured -> runtime_error") {
    Fixture f;
    std::string id = f.startedInstance();
    CHECK_THROWS_AS(f.facade.requestApproval(id, "1"), std::runtime_error);
    CHECK(contains(errorOf([&] { f.facade.requestApproval(id, "1"); }), "no approval chain"));
}

TEST_CASE("unknown instance -> invalid_argument (checked before the chain)") {
    Fixture f;
    CHECK_THROWS_AS(f.facade.requestApproval("INST-9", "1"), std::invalid_argument);
}

TEST_CASE("unknown task -> invalid_argument") {
    Fixture f;
    std::string id = approvalInstance(f, "1");
    CHECK_THROWS_AS(f.facade.requestApproval(id, "77"), std::invalid_argument);
}

TEST_CASE("empty IDs -> invalid_argument") {
    Fixture f;
    std::string id = approvalInstance(f, "1");
    CHECK_THROWS_AS(f.facade.requestApproval("", "1"), std::invalid_argument);
    CHECK_THROWS_AS(f.facade.requestApproval(id, ""), std::invalid_argument);
}

} // TEST_SUITE

TEST_SUITE("Facade/CoR: side effects and independence") {

TEST_CASE("requesting approval does not change the task's lifecycle state") {
    Fixture f;
    std::string id = approvalInstance(f, "2");
    const std::size_t before = f.facade.getAuditLog().size();
    f.facade.requestApproval(id, "1");
    CHECK(f.facade.getAuditLog().size() == before);             // no state-change event
    CHECK_NOTHROW(f.facade.assignTask(id, "1", "alice"));        // still Available
}

TEST_CASE("approval requests are repeatable and give the same answer each time") {
    Fixture f;
    std::string id = approvalInstance(f, "2");
    for (int i = 0; i < 10; ++i) {
        CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
    }
}

TEST_CASE("approval is not recorded as an undoable command") {
    Fixture f;
    std::string id = approvalInstance(f, "1");
    f.facade.requestApproval(id, "1");
    CHECK_FALSE(f.facade.undoLastAction());
}

TEST_CASE("approval can be requested at any lifecycle stage, even before the instance starts") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("appr", &f.workerRole, 1);
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("appr");   // NOT started
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
}

TEST_CASE("each task carries its own required level") {
    Fixture f;
    WorkflowDefinition* def = makeParallelDefinition("mixed", &f.workerRole, 2);
    std::vector<WorkItem*> kids = def->getRoot()->getChildren();
    kids[0]->setData("requiredApprovalLevel", "1");
    kids[1]->setData("requiredApprovalLevel", "9");
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string id = f.facade.createInstance("mixed");
    f.facade.startInstance(id);
    CHECK(f.facade.requestApproval(id, "1") == ApprovalStatus::Approved);
    CHECK(f.facade.requestApproval(id, "2") == ApprovalStatus::Escalated);
}

TEST_CASE("different definitions use their own chains") {
    Fixture f;
    WorkflowDefinition* strict = makeSequentialDefinition("strict", &f.workerRole, 1);
    strict->setApprovalChain(new ApprovalLevel(&f.nurseRole, 60));      // weak chain
    tagTasks(strict, "requiredApprovalLevel", "2");
    WorkflowDefinition* lax = makeSequentialDefinition("lax", &f.workerRole, 1);
    lax->setApprovalChain(new ApprovalLevel(&f.doctorRole, 60));        // strong chain
    tagTasks(lax, "requiredApprovalLevel", "2");
    f.facade.registerDefinition(strict);
    f.facade.registerDefinition(lax);
    std::string s = f.facade.createInstance("strict");
    std::string l = f.facade.createInstance("lax");
    CHECK(f.facade.requestApproval(s, "1") == ApprovalStatus::Escalated);
    CHECK(f.facade.requestApproval(l, "1") == ApprovalStatus::Approved);
}

TEST_CASE("two instances of one definition share the chain but not the request state") {
    Fixture f;
    WorkflowDefinition* def = makeSequentialDefinition("appr", &f.workerRole, 1);
    tagTasks(def, "requiredApprovalLevel", "9");
    def->setApprovalChain(makeApprovalChain(&f.nurseRole, &f.doctorRole));
    f.facade.registerDefinition(def);
    std::string a = f.facade.createInstance("appr");
    std::string b = f.facade.createInstance("appr");
    CHECK(f.facade.requestApproval(a, "1") == ApprovalStatus::Escalated);
    CHECK(f.facade.requestApproval(b, "1") == ApprovalStatus::Escalated);   // not "already decided"
}

TEST_CASE("approving a Stage item falls back to the default level (characterisation)") {
    Fixture f;
    std::string id = approvalInstance(f, "9");
    CHECK(f.facade.requestApproval(id, "0") == ApprovalStatus::Approved);   // stage has no data key
}

} // TEST_SUITE