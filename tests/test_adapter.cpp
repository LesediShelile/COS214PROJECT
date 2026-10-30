
#include "doctest.h"

#include "AdapterOne.h"
#include "AdapterTwo.h"
#include "APIOne.h"
#include "APITwo.h"
#include "ExternalConnector.h"

#include <stdexcept>
#include <string>

TEST_SUITE("Adapter Pattern") {

    TEST_CASE("AdapterOne implements ExternalConnector") {
        APIOne api;
        AdapterOne adapter(&api);

        ExternalConnector* connector = &adapter;

        CHECK(connector->getName() == "API One");
    }

    TEST_CASE("AdapterTwo implements ExternalConnector") {
        APITwo api;
        AdapterTwo adapter(&api);

        ExternalConnector* connector = &adapter;

        CHECK(connector->getName() == "API Two");
    }

    TEST_CASE("AdapterOne supports adding entries") {
        APIOne api;
        AdapterOne adapter(&api);

        std::string operation = "add";
        std::string payload = "task1=Complete assignment";

        CHECK_NOTHROW(
            adapter.request(&operation, &payload)
        );
    }

    TEST_CASE("AdapterOne supports removing entries") {
        APIOne api;
        AdapterOne adapter(&api);

        std::string addOperation = "add";
        std::string addPayload = "task1=Complete assignment";

        adapter.request(&addOperation, &addPayload);

        std::string removeOperation = "remove";
        std::string removePayload = "task1";

        CHECK_NOTHROW(
            adapter.request(&removeOperation, &removePayload)
        );
    }

    TEST_CASE("AdapterOne rejects malformed add payload") {
        APIOne api;
        AdapterOne adapter(&api);

        std::string operation = "add";
        std::string payload = "invalidPayload";

        CHECK_THROWS_AS(
            adapter.request(&operation, &payload),
            std::invalid_argument
        );
    }

    TEST_CASE("AdapterOne rejects unsupported operations") {
        APIOne api;
        AdapterOne adapter(&api);

        std::string operation = "unknown";
        std::string payload = "data";

        CHECK_THROWS_AS(
            adapter.request(&operation, &payload),
            std::invalid_argument
        );
    }

    TEST_CASE("AdapterTwo supports add operation") {
        APITwo api;
        AdapterTwo adapter(&api);

        std::string operation = "add";
        std::string payload = "Task information";

        CHECK_NOTHROW(
            adapter.request(&operation, &payload)
        );
    }

    TEST_CASE("AdapterTwo supports alert operation") {
        APITwo api;
        AdapterTwo adapter(&api);

        std::string operation = "alert";
        std::string payload = "Urgent task";

        CHECK_NOTHROW(
            adapter.request(&operation, &payload)
        );
    }

    TEST_CASE("AdapterTwo supports dispatch operation") {
        APITwo api;
        AdapterTwo adapter(&api);

        std::string operation = "dispatch";
        std::string payload = "Workflow update";

        CHECK_NOTHROW(
            adapter.request(&operation, &payload)
        );
    }

    TEST_CASE("AdapterTwo rejects unsupported operations") {
        APITwo api;
        AdapterTwo adapter(&api);

        std::string operation = "unknown";
        std::string payload = "data";

        CHECK_THROWS_AS(
            adapter.request(&operation, &payload),
            std::invalid_argument
        );
    }

    TEST_CASE("Adapters reject null request arguments") {
        APIOne api;
        AdapterOne adapter(&api);

        std::string operation = "add";

        CHECK_THROWS_AS(
            adapter.request(nullptr, &operation),
            std::invalid_argument
        );

        CHECK_THROWS_AS(
            adapter.request(&operation, nullptr),
            std::invalid_argument
        );
    }

    TEST_CASE("Adapters reject null API objects") {
        CHECK_THROWS_AS(
            AdapterOne(nullptr),
            std::invalid_argument
        );

        CHECK_THROWS_AS(
            AdapterTwo(nullptr),
            std::invalid_argument
        );
    }
}
