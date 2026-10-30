
#include "third_party/doctest.h"

#include "../include/WorkflowEvent.h"
#include "../include/EventPublisher.h"
#include "../include/WorkflowListener.h"
#include "../include/AuditLogger.h"
#include "../include/MetricsCollector.h"
#include "../include/Notifier.h"
#include "../include/EngineRouter.h"
#include "../include/WorkflowDefinition.h"
#include "../include/WorkflowInstance.h"
#include "../include/SequentialStage.h"

#include <string>
#include <vector>

namespace {

class TestListener : public WorkflowListener {
public:
    int notificationCount = 0;
    std::vector<std::string> receivedDetails;
    std::vector<EventType> receivedTypes;

    void update(WorkflowEvent* event) override {
        if (event != nullptr) {
            notificationCount++;
            receivedDetails.push_back(event->getDetails());
            receivedTypes.push_back(event->getType());
        }
    }
};

}

// ============================================================
// WORKFLOW EVENT TESTS
// ============================================================

TEST_SUITE("Workflow Event") {

    TEST_CASE("Event stores and returns all supplied information") {
        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "TASK-01",
            "2026-10-30 10:00:00",
            "Task completed"
        );

        CHECK(event.getType() == EventType::ITEM_COMPLETED);
        CHECK(event.getSourceID() == "TASK-01");
        CHECK(event.getTimestamp() == "2026-10-30 10:00:00");
        CHECK(event.getDetails() == "Task completed");
    }

    TEST_CASE("Event supports different event types") {
        WorkflowEvent stateEvent(
            EventType::ITEM_STATE_CHANGED,
            "TASK-02",
            "10:00",
            "Available"
        );

        WorkflowEvent timeoutEvent(
            EventType::TIMEOUT,
            "TASK-03",
            "10:30",
            "Approval expired"
        );

        CHECK(stateEvent.getType() == EventType::ITEM_STATE_CHANGED);
        CHECK(timeoutEvent.getType() == EventType::TIMEOUT);
    }

    TEST_CASE("Event supports empty details") {
        WorkflowEvent event(
            EventType::EXTERNAL_EVENT,
            "EXT-01",
            "11:00",
            ""
        );

        CHECK(event.getDetails().empty());
    }
}

// ============================================================
// EVENT PUBLISHER TESTS
// ============================================================

TEST_SUITE("Event Publisher") {

    TEST_CASE("Publisher notifies an attached listener") {
        EventPublisher publisher;
        TestListener listener;

        publisher.attach(&listener);

        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:00",
            "Completed"
        );

        publisher.publish(&event);

        CHECK(listener.notificationCount == 1);
        CHECK(listener.receivedDetails[0] == "Completed");
    }

    TEST_CASE("Publisher notifies multiple listeners") {
        EventPublisher publisher;

        TestListener first;
        TestListener second;
        TestListener third;

        publisher.attach(&first);
        publisher.attach(&second);
        publisher.attach(&third);

        WorkflowEvent event(
            EventType::TASK_ACTION,
            "T1",
            "10:00",
            "Task updated"
        );

        publisher.publish(&event);

        CHECK(first.notificationCount == 1);
        CHECK(second.notificationCount == 1);
        CHECK(third.notificationCount == 1);
    }

    TEST_CASE("Publisher does not notify detached listeners") {
        EventPublisher publisher;
        TestListener listener;

        publisher.attach(&listener);
        publisher.detach(&listener);

        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:00",
            "Completed"
        );

        publisher.publish(&event);

        CHECK(listener.notificationCount == 0);
    }

    TEST_CASE("Detaching one listener does not affect others") {
        EventPublisher publisher;

        TestListener first;
        TestListener second;

        publisher.attach(&first);
        publisher.attach(&second);

        publisher.detach(&first);

        WorkflowEvent event(
            EventType::TASK_ACTION,
            "T1",
            "10:00",
            "Updated"
        );

        publisher.publish(&event);

        CHECK(first.notificationCount == 0);
        CHECK(second.notificationCount == 1);
    }

    TEST_CASE("Publisher safely ignores null listener") {
        EventPublisher publisher;
        TestListener listener;

        publisher.attach(nullptr);
        publisher.attach(&listener);

        WorkflowEvent event(
            EventType::EXTERNAL_EVENT,
            "EXT-01",
            "10:00",
            "External update"
        );

        CHECK_NOTHROW(publisher.publish(&event));
        CHECK(listener.notificationCount == 1);
    }

    TEST_CASE("Publisher safely ignores null event") {
        EventPublisher publisher;
        TestListener listener;

        publisher.attach(&listener);

        CHECK_NOTHROW(publisher.publish(nullptr));

        CHECK(listener.notificationCount == 0);
    }

    TEST_CASE("Publisher does not notify listeners after detachment") {
        EventPublisher publisher;
        TestListener listener;

        publisher.attach(&listener);
        publisher.detach(&listener);

        WorkflowEvent first(
            EventType::TASK_ACTION,
            "T1",
            "10:00",
            "First"
        );

        WorkflowEvent second(
            EventType::TASK_ACTION,
            "T1",
            "10:01",
            "Second"
        );

        publisher.publish(&first);
        publisher.publish(&second);

        CHECK(listener.notificationCount == 0);
    }
}

// ============================================================
// AUDIT LOGGER TESTS
// ============================================================

TEST_SUITE("Audit Logger") {

    TEST_CASE("AuditLogger records event details") {
        AuditLogger logger;

        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:00",
            "Task completed"
        );

        logger.update(&event);

        std::vector<std::string> records = logger.getRecords();

        REQUIRE(records.size() == 1);
        CHECK(records[0] == "Task completed");
    }

    TEST_CASE("AuditLogger records multiple events in order") {
        AuditLogger logger;

        WorkflowEvent first(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:00",
            "Available"
        );

        WorkflowEvent second(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:30",
            "Completed"
        );

        logger.update(&first);
        logger.update(&second);

        std::vector<std::string> records = logger.getRecords();

        REQUIRE(records.size() == 2);

        CHECK(records[0] == "Available");
        CHECK(records[1] == "Completed");
    }

    TEST_CASE("AuditLogger safely ignores null events") {
        AuditLogger logger;

        CHECK_NOTHROW(logger.update(nullptr));

        CHECK(logger.getRecords().empty());
    }

    TEST_CASE("AuditLogger receives events through publisher") {
        EventPublisher publisher;
        AuditLogger logger;

        publisher.attach(&logger);

        WorkflowEvent event(
            EventType::TASK_ACTION,
            "T1",
            "10:00",
            "Priority changed"
        );

        publisher.publish(&event);

        std::vector<std::string> records = logger.getRecords();

        REQUIRE(records.size() == 1);
        CHECK(records[0] == "Priority changed");
    }
}

// ============================================================
// METRICS COLLECTOR TESTS
// ============================================================

TEST_SUITE("Metrics Collector") {

    TEST_CASE("MetricsCollector counts state transitions") {
        MetricsCollector metrics;

        WorkflowEvent event(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:00",
            "Available"
        );

        metrics.update(&event);

        CHECK(metrics.getTransitionCount("Available") == 1);
    }

    TEST_CASE("MetricsCollector counts repeated transitions") {
        MetricsCollector metrics;

        WorkflowEvent first(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:00",
            "Assigned"
        );

        WorkflowEvent second(
            EventType::ITEM_STATE_CHANGED,
            "T2",
            "10:05",
            "Assigned"
        );

        WorkflowEvent third(
            EventType::ITEM_STATE_CHANGED,
            "T3",
            "10:10",
            "Assigned"
        );

        metrics.update(&first);
        metrics.update(&second);
        metrics.update(&third);

        CHECK(metrics.getTransitionCount("Assigned") == 3);
    }

    TEST_CASE("MetricsCollector tracks different states separately") {
        MetricsCollector metrics;

        WorkflowEvent available(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:00",
            "Available"
        );

        WorkflowEvent assigned(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:05",
            "Assigned"
        );

        metrics.update(&available);
        metrics.update(&assigned);

        CHECK(metrics.getTransitionCount("Available") == 1);
        CHECK(metrics.getTransitionCount("Assigned") == 1);
    }

    TEST_CASE("Unknown states return zero transitions") {
        MetricsCollector metrics;

        CHECK(metrics.getTransitionCount("Unknown") == 0);
    }

    TEST_CASE("MetricsCollector safely ignores null events") {
        MetricsCollector metrics;

        CHECK_NOTHROW(metrics.update(nullptr));

        CHECK(metrics.getTransitionCount("Available") == 0);
    }

    TEST_CASE("MetricsCollector produces a summary") {
        MetricsCollector metrics;

        WorkflowEvent event(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:00",
            "Completed"
        );

        metrics.update(&event);

        std::string summary = metrics.getSummary();

        CHECK(summary.find("Completed") != std::string::npos);
        CHECK(summary.find("Transitions: 1") != std::string::npos);
    }

    TEST_CASE("MetricsCollector returns zero average when no duration exists") {
        MetricsCollector metrics;

        CHECK(metrics.getAverageTimeInState("Available") == 0.0f);
    }

    TEST_CASE("MetricsCollector receives events through publisher") {
        EventPublisher publisher;
        MetricsCollector metrics;

        publisher.attach(&metrics);

        WorkflowEvent event(
            EventType::ITEM_STATE_CHANGED,
            "T1",
            "10:00",
            "Started"
        );

        publisher.publish(&event);

        CHECK(metrics.getTransitionCount("Started") == 1);
    }
}

// ============================================================
// NOTIFIER TESTS
// ============================================================

TEST_SUITE("Notifier") {

    TEST_CASE("Notifier safely receives workflow events") {
        Notifier notifier;

        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:00",
            "Task completed"
        );

        CHECK_NOTHROW(notifier.update(&event));
    }

    TEST_CASE("Notifier safely handles null events") {
        Notifier notifier;

        CHECK_NOTHROW(notifier.update(nullptr));
    }

    TEST_CASE("Notifier can be attached to publisher") {
        EventPublisher publisher;
        Notifier notifier;

        publisher.attach(&notifier);

        WorkflowEvent event(
            EventType::TIMEOUT,
            "T1",
            "10:00",
            "Approval timed out"
        );

        CHECK_NOTHROW(publisher.publish(&event));
    }
}

// ============================================================
// ENGINE ROUTER TESTS
// ============================================================

TEST_SUITE("Engine Router") {

    TEST_CASE("EngineRouter safely ignores null events") {
        WorkflowDefinition definition("Test", 1);

        definition.setRoot(
            new SequentialStage("S1", "Main")
        );

        definition.freeze();

        WorkflowInstance instance("I1", &definition);
        EngineRouter router(&instance, nullptr);

        CHECK_NOTHROW(router.update(nullptr));

        CHECK(instance.getStatus() == WorkflowStatus::CREATED);
    }

    TEST_CASE("EngineRouter ignores events other than ITEM_COMPLETED") {
        WorkflowDefinition definition("Test", 1);

        definition.setRoot(
            new SequentialStage("S1", "Main")
        );

        definition.freeze();

        WorkflowInstance instance("I1", &definition);
        instance.start();

        EngineRouter router(&instance, nullptr);

        WorkflowEvent event(
            EventType::TASK_ACTION,
            "T1",
            "10:00",
            "Task updated"
        );

        router.update(&event);

        CHECK(instance.getStatus() == WorkflowStatus::RUNNING);
    }

    TEST_CASE("EngineRouter completes an empty workflow after ITEM_COMPLETED") {
        WorkflowDefinition definition("Test", 1);

        definition.setRoot(
            new SequentialStage("S1", "Main")
        );

        definition.freeze();

        WorkflowInstance instance("I1", &definition);
        instance.start();

        EngineRouter router(&instance, nullptr);

        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:00",
            "Completed"
        );

        router.update(&event);

        CHECK(instance.getStatus() == WorkflowStatus::COMPLETED);
        CHECK(instance.isFinished());
    }

    TEST_CASE("EngineRouter ignores events when instance is not running") {
        WorkflowDefinition definition("Test", 1);

        definition.setRoot(
            new SequentialStage("S1", "Main")
        );

        definition.freeze();

        WorkflowInstance instance("I1", &definition);
        EngineRouter router(&instance, nullptr);

        WorkflowEvent event(
            EventType::ITEM_COMPLETED,
            "T1",
            "10:00",
            "Completed"
        );

        router.update(&event);

        CHECK(instance.getStatus() == WorkflowStatus::CREATED);
    }
}

