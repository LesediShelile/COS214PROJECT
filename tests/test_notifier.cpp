#include "third_party/doctest.h"

#include "../include/Notifier.h"
#include "../include/WorkflowEvent.h"
#include "../include/EventPublisher.h"
#include "../include/EventType.h"

#include <string>
#include <vector>

TEST_SUITE("Notifier comprehensive tests") {

    TEST_CASE("Notifier formats every supported workflow event type") {
        Notifier notifier;

        WorkflowEvent stateChanged(
            EventType::ITEM_STATE_CHANGED, "TASK-1", "10:00", "Started");
        WorkflowEvent itemCompleted(
            EventType::ITEM_COMPLETED, "TASK-2", "10:01", "Completed");
        WorkflowEvent taskAction(
            EventType::TASK_ACTION, "TASK-3", "10:02", "Assigned");
        WorkflowEvent externalEvent(
            EventType::EXTERNAL_EVENT, "EXT-1", "10:03", "API response received");
        WorkflowEvent timeout(
            EventType::TIMEOUT, "APP-1", "10:04", "Approval expired");
        WorkflowEvent workflowFinished(
            EventType::WORKFLOW_FINISHED, "WF-1", "10:05", "Success");

        notifier.update(&stateChanged);
        notifier.update(&itemCompleted);
        notifier.update(&taskAction);
        notifier.update(&externalEvent);
        notifier.update(&timeout);
        notifier.update(&workflowFinished);

        std::vector<std::string> messages = notifier.getNotifications();
        REQUIRE(messages.size() == 6);
        CHECK(messages[0].find("Item state changed") != std::string::npos);
        CHECK(messages[1].find("Item completed") != std::string::npos);
        CHECK(messages[2].find("Task action performed") != std::string::npos);
        CHECK(messages[3].find("External event received") != std::string::npos);
        CHECK(messages[4].find("Approval timeout") != std::string::npos);
        CHECK(messages[5].find("Workflow finished") != std::string::npos);

        CHECK(messages[0].find("Source: TASK-1") != std::string::npos);
        CHECK(messages[0].find("Details: Started") != std::string::npos);
        CHECK(messages[0].find("Time: 10:00") != std::string::npos);
        CHECK(notifier.getNotificationCount() == 6);
    }

    TEST_CASE("Notifier ignores null events without changing its notifications") {
        Notifier notifier;
        WorkflowEvent event(EventType::TASK_ACTION, "T1", "10:00", "Started");

        notifier.update(&event);
        notifier.update(nullptr);

        CHECK(notifier.getNotificationCount() == 1);
        CHECK(notifier.getNotifications().size() == 1);
    }

    TEST_CASE("Notifier returns an empty collection before receiving events") {
        Notifier notifier;

        CHECK(notifier.getNotifications().empty());
        CHECK(notifier.getNotificationCount() == 0);
    }

    TEST_CASE("Notifier clear removes all stored notifications") {
        Notifier notifier;
        WorkflowEvent first(EventType::ITEM_COMPLETED, "T1", "10:00", "Done");
        WorkflowEvent second(EventType::TIMEOUT, "A1", "10:01", "Expired");

        notifier.update(&first);
        notifier.update(&second);
        REQUIRE(notifier.getNotificationCount() == 2);

        notifier.clearNotifications();

        CHECK(notifier.getNotifications().empty());
        CHECK(notifier.getNotificationCount() == 0);
    }

    TEST_CASE("Notifier can receive new events after being cleared") {
        Notifier notifier;
        WorkflowEvent first(EventType::ITEM_COMPLETED, "T1", "10:00", "Done");
        WorkflowEvent next(EventType::WORKFLOW_FINISHED, "WF1", "10:01", "Finished");

        notifier.update(&first);
        notifier.clearNotifications();
        notifier.update(&next);

        REQUIRE(notifier.getNotificationCount() == 1);
        CHECK(notifier.getNotifications()[0].find("Workflow finished") != std::string::npos);
    }

    TEST_CASE("Notifier receives events when registered with EventPublisher") {
        EventPublisher publisher;
        Notifier notifier;
        publisher.attach(&notifier);

        WorkflowEvent event(EventType::EXTERNAL_EVENT, "EXT-2", "11:00", "Updated");
        publisher.publish(&event);

        REQUIRE(notifier.getNotificationCount() == 1);
        CHECK(notifier.getNotifications()[0].find("External event received") != std::string::npos);
    }
}
