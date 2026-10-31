#include "third_party/doctest.h"

#include "../include/ExternalTask.h"
#include "../include/ExternalConnector.h"
#include "../include/Participant.h"

#include <stdexcept>
#include <string>

namespace {

class MockExternalConnector : public ExternalConnector {
public:
    int requestCalls;
    std::string receivedOperation;
    std::string receivedPayload;
    std::string serviceName;
    bool throwOnRequest;
    bool throwOnGetName;

    explicit MockExternalConnector(const std::string& name = "Mock Service")
        : requestCalls(0), receivedOperation(), receivedPayload(),
          serviceName(name), throwOnRequest(false), throwOnGetName(false) {}

    void request(const std::string* operation,
                 const std::string* payload) override {
        ++requestCalls;

        if (throwOnRequest) {
            throw std::runtime_error("request failed");
        }

        receivedOperation = operation ? *operation : "<null>";
        receivedPayload = payload ? *payload : "<null>";
    }

    std::string getName() override {
        if (throwOnGetName) {
            throw std::runtime_error("could not retrieve service name");
        }
        return serviceName;
    }
};

} // namespace

TEST_SUITE("ExternalTask") {

    TEST_CASE("Construction stores task details and starts with an empty response") {
        MockExternalConnector connector;
        ExternalTask task("external-1", "Send request", 0, 0,
                           &connector, "submit-record");

        CHECK(task.getItemID() == "external-1");
        CHECK(task.getName() == "Send request");
        CHECK(task.getResponse().empty());
        CHECK(task.getStateName() == "Created");
    }

    TEST_CASE("execute sends the request to the connector and stores its name") {
        MockExternalConnector connector("Records API");
        ExternalTask task("external-2", "Create record", 0, 0,
                           &connector, "create:record-42");

        CHECK(task.execute());
        CHECK(connector.requestCalls == 1);
        CHECK(connector.receivedOperation == "create:record-42");
        CHECK(connector.receivedPayload == "create:record-42");
        CHECK(task.getResponse() == "Request submitted to Records API");
    }

    TEST_CASE("execute returns false when the connector is null") {
        ExternalTask task("external-3", "No connector", 0, 0,
                           0, "some-request");

        CHECK_FALSE(task.execute());
        CHECK(task.getResponse().empty());
    }

    TEST_CASE("execute returns false when the request is empty") {
        MockExternalConnector connector;
        ExternalTask task("external-4", "Empty request", 0, 0,
                           &connector, "");

        CHECK_FALSE(task.execute());
        CHECK(connector.requestCalls == 0);
        CHECK(task.getResponse().empty());
    }

    TEST_CASE("setRequest replaces the request used by the next execution") {
        MockExternalConnector connector;
        ExternalTask task("external-5", "Change request", 0, 0,
                           &connector, "old-request");

        task.setRequest("new-request");

        CHECK(task.execute());
        CHECK(connector.receivedOperation == "new-request");
        CHECK(connector.receivedPayload == "new-request");
    }

    TEST_CASE("setRequest can replace an empty request with a valid request") {
        MockExternalConnector connector;
        ExternalTask task("external-6", "Repair request", 0, 0,
                           &connector, "");

        CHECK_FALSE(task.execute());
        task.setRequest("retry-request");

        CHECK(task.execute());
        CHECK(connector.requestCalls == 1);
        CHECK(connector.receivedPayload == "retry-request");
    }

    TEST_CASE("clone creates another ExternalTask with the same configuration and response") {
        MockExternalConnector connector("Clone API");
        ExternalTask original("external-7", "Clone me", 0, 0,
                              &connector, "clone-request");

        CHECK(original.execute());

        WorkItem* clonedItem = original.clone();
        ExternalTask* clone = dynamic_cast<ExternalTask*>(clonedItem);

        REQUIRE(clone != 0);
        CHECK(clone->getItemID() == original.getItemID());
        CHECK(clone->getName() == original.getName());
        CHECK(clone->getResponse() == original.getResponse());

        // The clone shares the non-owning connector but has its own request value.
        clone->setRequest("clone-only-request");
        CHECK(clone->execute());
        CHECK(connector.receivedPayload == "clone-only-request");

        // Changing the clone's request must not change the original's request.
        CHECK(original.execute());
        CHECK(connector.receivedPayload == "clone-request");

        delete clonedItem;
    }

    TEST_CASE("clone preserves the current Task state independently") {
        MockExternalConnector connector;
        ExternalTask original("external-8", "State clone", 0, 0,
                              &connector, "state-request");
        original.makeAvailable();

        WorkItem* clonedItem = original.clone();
        REQUIRE(clonedItem != 0);
        CHECK(clonedItem->getStateName() == original.getStateName());

        Participant participant("p1", "Test Worker", 0);
        clonedItem->assign(&participant);
        CHECK(clonedItem->getStateName() == "Assigned");
        CHECK(original.getStateName() == "Available");

        delete clonedItem;
    }

    TEST_CASE("execute propagates an exception thrown by the connector") {
        MockExternalConnector connector;
        connector.throwOnRequest = true;
        ExternalTask task("external-9", "Failing request", 0, 0,
                           &connector, "will-fail");

        CHECK_THROWS_AS(task.execute(), std::runtime_error);
        CHECK(connector.requestCalls == 1);
        CHECK(task.getResponse().empty());
    }

    TEST_CASE("execute propagates an exception thrown while obtaining connector name") {
        MockExternalConnector connector;
        connector.throwOnGetName = true;
        ExternalTask task("external-10", "Name failure", 0, 0,
                           &connector, "valid-request");

        CHECK_THROWS_AS(task.execute(), std::runtime_error);
        CHECK(connector.requestCalls == 1);
        CHECK(task.getResponse().empty());
    }

    TEST_CASE("a successful execution can be repeated and updates the response") {
        MockExternalConnector connector("First API");
        ExternalTask task("external-11", "Repeat request", 0, 0,
                           &connector, "repeat-request");

        CHECK(task.execute());
        CHECK(task.getResponse() == "Request submitted to First API");

        connector.serviceName = "Second API";
        CHECK(task.execute());
        CHECK(connector.requestCalls == 2);
        CHECK(task.getResponse() == "Request submitted to Second API");
    }
}
