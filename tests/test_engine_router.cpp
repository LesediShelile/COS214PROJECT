#include "../third_party/doctest.h"
#include "../include/EngineRouter.h"
#include "../include/ApprovalRequest.h"
#include "../include/WorkflowEvent.h"
#include "../include/WorkflowDefinition.h"
#include "../include/SequentialStage.h"
#include "../include/WorkItem.h"

class RecordingApprovalHandler : public ApprovalHandler {
public:
    RecordingApprovalHandler() : calls(0), lastRequest(nullptr) {}
    void handle(ApprovalRequest* request) override {
        ++calls;
        lastRequest = request;
    }
    int calls;
    ApprovalRequest* lastRequest;
};

TEST_SUITE("EngineRouter additional coverage") {
    TEST_CASE("TIMEOUT is forwarded to the approval chain") {
        WorkflowDefinition definition("Router timeout", 1);
        definition.setRoot(new SequentialStage("S1", "Main"));
        definition.freeze();
        WorkflowInstance instance("I-timeout", &definition);

        RecordingApprovalHandler handler;
        ApprovalRequest request(nullptr, 1, 100);
        EngineRouter router(&instance, &handler);
        WorkflowEvent event(EventType::TIMEOUT, "approval-1", "10:00", "Timed out", &request);

        router.update(&event);

        CHECK(handler.calls == 1);
        CHECK(handler.lastRequest == &request);
        CHECK(instance.getStatus() == WorkflowStatus::CREATED);
    }

    TEST_CASE("TIMEOUT with no approval request is safely ignored") {
        WorkflowDefinition definition("Router timeout without request", 1);
        definition.setRoot(new SequentialStage("S1", "Main"));
        definition.freeze();
        WorkflowInstance instance("I-timeout-null", &definition);
        RecordingApprovalHandler handler;
        EngineRouter router(&instance, &handler);
        WorkflowEvent event(EventType::TIMEOUT, "approval-2", "10:00", "Timed out", nullptr);

        CHECK_NOTHROW(router.update(&event));
        CHECK(handler.calls == 0);
    }

    TEST_CASE("TIMEOUT with no approval chain is safely ignored") {
        WorkflowDefinition definition("Router no chain", 1);
        definition.setRoot(new SequentialStage("S1", "Main"));
        definition.freeze();
        WorkflowInstance instance("I-no-chain", &definition);
        ApprovalRequest request(nullptr, 1, 100);
        EngineRouter router(&instance, nullptr);
        WorkflowEvent event(EventType::TIMEOUT, "approval-3", "10:00", "Timed out", &request);

        CHECK_NOTHROW(router.update(&event));
        CHECK(instance.getStatus() == WorkflowStatus::CREATED);
    }

    TEST_CASE("non-timeout event is ignored when router has no instance") {
        EngineRouter router(nullptr, nullptr);
        WorkflowEvent event(EventType::ITEM_COMPLETED, "task-1", "10:00", "Completed");
        CHECK_NOTHROW(router.update(&event));
    }
}
