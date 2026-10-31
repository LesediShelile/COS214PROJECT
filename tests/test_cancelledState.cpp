#include "third_party/doctest.h"

#include "../include/CancelledState.h"
#include "../include/TaskState.h"

#include <string>

TEST_SUITE("Cancelled state direct tests") {

    TEST_CASE("Cancelled state reports its exact name") {
        Cancelled state;
        CHECK(state.getName() == "Cancelled");
    }

    TEST_CASE("Cancelled state is a final state") {
        Cancelled state;
        CHECK(state.isFinal());
        CHECK_FALSE(state.isAvailable());
    }

    TEST_CASE("Cancelled state clone is a separate equivalent object") {
        Cancelled original;
        TaskState* copy = original.clone();

        REQUIRE(copy != nullptr);
        CHECK(copy != &original);
        CHECK(copy->getName() == "Cancelled");
        CHECK(copy->isFinal());
        CHECK_FALSE(copy->isAvailable());

        delete copy;
    }
}
